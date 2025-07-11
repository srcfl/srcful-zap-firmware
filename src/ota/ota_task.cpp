#include "ota_task.h"
#include "../firmware_version.h"
#include <algorithm>
#include "../zap_log.h" // Added for logging

// Define TAG for logging
static const char* TAG = "ota_task";

OTATask::OTATask(uint32_t stackSize, UBaseType_t priority) 
    : taskHandle(nullptr), stackSize(stackSize), priority(priority), shouldRun(false),
      updateInProgress(false), progressPercent(0), resultAvailable(false) {
    
    // Create the queue for update requests (store just 1 request)
    updateQueue = xQueueCreate(1, sizeof(OTAUpdateRequest));
    if (updateQueue == nullptr) {
        LOG_E(TAG, "Failed to create queue");
    }
    
    // Create mutex for protecting status variables
    statusMutex = xSemaphoreCreateMutex();
    if (statusMutex == nullptr) {
        LOG_E(TAG, "Failed to create mutex");
    }
    
    // Initialize update result
    memset(&lastResult, 0, sizeof(lastResult));
}

OTATask::~OTATask() {
    stop();
    
    // Delete queue if it exists
    if (updateQueue != nullptr) {
        vQueueDelete(updateQueue);
        updateQueue = nullptr;
    }
    
    // Delete mutex if it exists
    if (statusMutex != nullptr) {
        vSemaphoreDelete(statusMutex);
        statusMutex = nullptr;
    }
}

void OTATask::begin() {
    if (taskHandle != nullptr) {
        return; // Task already running
    }
    
    shouldRun = true;
    xTaskCreatePinnedToCore(
        taskFunction,
        "OTATask",
        stackSize,
        this,
        priority,
        &taskHandle,
        0  // Run on core 0
    );
    
    LOG_I(TAG, "Started");
}

void OTATask::stop() {
    if (taskHandle == nullptr) {
        return; // Task not running
    }
    
    shouldRun = false;
    vTaskDelay(pdMS_TO_TICKS(100)); // Give the task time to exit
    
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    
    LOG_I(TAG, "Stopped");
}

bool OTATask::requestUpdate(const zap::Str& url, const zap::Str& version) {
    if (updateQueue == nullptr) {
        LOG_E(TAG, "Queue not created");
        return false;
    }
    
    // Check if an update is already in progress
    if (isUpdateInProgress()) {
        LOG_W(TAG, "Update already in progress");
        return false;
    }
    
    // Create request
    OTAUpdateRequest request;
    memset(&request, 0, sizeof(request));
    
    // Copy URL
    strncpy(request.url, url.c_str(), sizeof(request.url) - 1);
    
    // Copy version if provided
    if (version.length() > 0) {
        strncpy(request.version, version.c_str(), sizeof(request.version) - 1);
        request.hasVersion = true;
    } else {
        request.hasVersion = false;
    }
    
    // Send to queue with a timeout of 500ms
    if (xQueueSend(updateQueue, &request, pdMS_TO_TICKS(500)) != pdPASS) {
        LOG_E(TAG, "Failed to send request to queue");
        return false;
    }
    
    LOG_I(TAG, "Update requested for URL: %s", url.c_str());
    return true;
}

bool OTATask::isUpdateInProgress() const {
    bool inProgress = false;
    
    if (statusMutex != nullptr) {
        if (xSemaphoreTake(statusMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            inProgress = updateInProgress;
            xSemaphoreGive(statusMutex);
        }
    }
    
    return inProgress;
}

bool OTATask::getUpdateResult(OTAUpdateResult* result) {
    bool success = false;
    
    if (result == nullptr || statusMutex == nullptr) {
        return false;
    }
    
    if (xSemaphoreTake(statusMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (resultAvailable) {
            memcpy(result, &lastResult, sizeof(OTAUpdateResult));
            resultAvailable = false;  // Clear the flag after reading
            success = true;
        }
        xSemaphoreGive(statusMutex);
    }
    
    return success;
}

int OTATask::getUpdateProgress() const {
    int progress = 0;
    
    if (statusMutex != nullptr) {
        if (xSemaphoreTake(statusMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            progress = progressPercent;
            xSemaphoreGive(statusMutex);
        }
    }
    
    return progress;
}

void OTATask::taskFunction(void* parameter) {
    OTATask* task = static_cast<OTATask*>(parameter);
    
    while (task->shouldRun) {
        // Check if there's a request in the queue
        OTAUpdateRequest request;
        if (xQueueReceive(task->updateQueue, &request, pdMS_TO_TICKS(500)) == pdTRUE) {
            LOG_I(TAG, "Received update request");
            
            // Set update in progress flag
            if (task->statusMutex != nullptr) {

                if (xSemaphoreTake(task->statusMutex, portMAX_DELAY)) {
                    task->updateInProgress = true;
                    task->progressPercent = 0;
                    xSemaphoreGive(task->statusMutex);
                }
            }
            
            // Perform the update
            OTAUpdateResult result;
            bool success = task->performUpdate(request, result);
            
            // Update the result
            if (task->statusMutex != nullptr) {
                if (xSemaphoreTake(task->statusMutex, portMAX_DELAY)) {
                    memcpy(&task->lastResult, &result, sizeof(OTAUpdateResult));
                    task->updateInProgress = false;
                    task->resultAvailable = true;
                    xSemaphoreGive(task->statusMutex);
                }
            }
            
            // If update was successful, we might reboot here
            if (success) {
                LOG_I(TAG, "Update successful, rebooting...");
                vTaskDelay(pdMS_TO_TICKS(1000)); // Give time for any final logging
                ESP.restart();
            }
        }
        
        // Yield for other tasks
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // Task cleanup
    vTaskDelete(NULL);
}

bool OTATask::performUpdate(const OTAUpdateRequest& request, OTAUpdateResult& result) {
    // Initialize result
    memset(&result, 0, sizeof(result));
    result.success = false;
    result.statusCode = 500; // Default error status
    
    LOG_I(TAG, "Starting update from URL: %s", request.url);
    
    // Check version if provided
    if (request.hasVersion) {
        String newVersion = String(request.version);
        String currentVersion = getFirmwareVersion();
        
        // Simple string comparison for now
        if (newVersion == currentVersion) {
            LOG_I(TAG, "Already running version %s", currentVersion.c_str());
            strncpy(result.message, "Already running latest version", sizeof(result.message) - 1);
            result.success = true;
            result.statusCode = 200;
            return false;  // Not an error, but no update needed
        }
        
        LOG_I(TAG, "Current version: %s", currentVersion.c_str());
        LOG_I(TAG, "New version: %s", newVersion.c_str());
    }
    
    LOG_D(TAG, "Free heap before update: %d", ESP.getFreeHeap());
    
    // Create HTTP client
    HTTPClient http;
    WiFiClientSecure client;
    
    // Configure SSL client to accept self-signed certificates
    client.setInsecure();
    
    // Begin the connection
    http.begin(client, request.url);
    
    // Get the firmware
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        LOG_E(TAG, "HTTP GET failed with code: %d", httpCode);
        http.end();
        
        snprintf(result.message, sizeof(result.message) - 1, 
                "HTTP GET failed with code: %d", httpCode);
        result.statusCode = httpCode;
        return false;
    }
    
    // Get the size of the firmware
    int contentLength = http.getSize();
    LOG_I(TAG, "Firmware size: %d bytes", contentLength);
    
    // Start the update
    if (!Update.begin(contentLength)) {
        LOG_E(TAG, "Failed to start update: %s", String(Update.getError()).c_str());
        http.end();
        
        snprintf(result.message, sizeof(result.message) - 1, 
                "Failed to start update size: %d", contentLength);
        return false;
    }
    
    // Get the stream
    WiFiClient* stream = http.getStreamPtr();
    
    // Write update data
    size_t written = 0;
    uint8_t buff[1024] = { 0 };
    
    while (written < contentLength) {
        if (!stream->connected()) {
            LOG_E(TAG, "Connection lost");
            http.end();
            
            strncpy(result.message, "Connection lost during update", sizeof(result.message) - 1);
            return false;
        }
        
        size_t available = stream->available();
        if (available) {
            size_t bytesToRead = std::min(available, (size_t)sizeof(buff));
            size_t bytesRead = stream->readBytes(buff, bytesToRead);
            
            if (bytesRead > 0) {
                if (Update.write(buff, bytesRead) != bytesRead) {
                    LOG_E(TAG, "Failed to write update data");
                    http.end();
                    
                    strncpy(result.message, "Failed to write update data", sizeof(result.message) - 1);
                    return false;
                }
                written += bytesRead;
                updateProgress(written, contentLength);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));  // Use vTaskDelay instead of delay for FreeRTOS compatibility
    }
    
    if (Update.end()) {
        LOG_I(TAG, "Update complete");
        http.end();
        
        strncpy(result.message, "Update successful", sizeof(result.message) - 1);
        result.success = true;
        result.statusCode = 200;
        return true;
    } else {
        LOG_E(TAG, "Update failed: %s", String(Update.getError()).c_str());
        http.end();
        
        snprintf(result.message, sizeof(result.message) - 1, 
                "Update failed: %d", Update.getError());
        return false;
    }
}

void OTATask::updateProgress(size_t current, size_t total) {
    int newPercent = (current * 100) / total;
    
    // Print progress every 5%
    static int lastPrintedPercent = 0;
    if (newPercent >= lastPrintedPercent + 5 || newPercent == 100) {
        LOG_D(TAG, "Progress: %d%%", newPercent);
        lastPrintedPercent = newPercent;
    }
    
    // Update the progress percentage in a thread-safe way
    if (statusMutex != nullptr) {
        if (xSemaphoreTake(statusMutex, portMAX_DELAY)) {
            progressPercent = newPercent;
            xSemaphoreGive(statusMutex);
        }
    }
}

bool OTATask::verifyFirmware(const uint8_t* data, size_t len) {
    // TODO: Implement firmware verification
    // This could include:
    // - Checking firmware signature
    // - Verifying firmware size
    // - Checking firmware version
    return true;
}