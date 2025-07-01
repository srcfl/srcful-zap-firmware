#!/usr/bin/env python3
"""
ESP32 Sequential Flashing Tool
Flashes firmware to ESP32 devices one at a time on a single port

Requirements:
    pip install pyserial esptool
    
Optional (for better port detection):
    pip install platformio
    
Usage:
    # Simple usage (auto-detects port and build files)
    python esp32_sequential_flasher.py
    
    # Specify build directory
    python esp32_sequential_flasher.py --build-dir .pio/build/esp32-c3
    
    # Specify port manually
    python esp32_sequential_flasher.py --port /dev/ttyUSB0
"""

import subprocess
import serial
import serial.tools.list_ports
import time
import json
import argparse
import sys
import csv  # Add csv import
from pathlib import Path
from typing import Dict, List, Optional, Tuple
import re
from datetime import datetime

class ESP32SequentialFlasher:
    def __init__(self, build_dir: str = None, flash_files: dict = None, baudrate: int = 115200):
        self.baudrate = baudrate
        self.results = []
        
        if flash_files:
            # Manual flash file specification
            self.flash_files = flash_files
            for addr, path in flash_files.items():
                if not Path(path).exists():
                    raise FileNotFoundError(f"Flash file not found: {path}")
        elif build_dir:
            # Auto-detect from specified PlatformIO build directory
            self.flash_files = self.detect_flash_files(build_dir)
        else:
            # Auto-detect from default PlatformIO build directories
            self.flash_files = self.auto_detect_build_dir()
    
    def auto_detect_build_dir(self) -> Dict[str, str]:
        """Auto-detect build directory and flash files from common PlatformIO locations"""
        # Common ESP32 environment names
        possible_envs = [
            'esp32-c3',      # Your specific case
            'esp32dev',      # Common default
            'esp32',         # Generic
            'nodemcu-32s',   # NodeMCU
            'esp32-s2',      # ESP32-S2
            'esp32-s3',      # ESP32-S3
        ]
        
        base_path = Path('.pio/build')
        
        if not base_path.exists():
            raise FileNotFoundError("No .pio/build directory found. Make sure to run 'pio run' first.")
        
        # Try each possible environment
        for env_name in possible_envs:
            env_path = base_path / env_name
            if env_path.exists():
                try:
                    flash_files = self.detect_flash_files(str(env_path))
                    print(f"Using build environment: {env_name}")
                    return flash_files
                except FileNotFoundError as e:
                    print(f"Warning: {e} for environment '{env_name}'. Trying next...")
        
        # If no common names work, list available environments
        available_envs = [d.name for d in base_path.iterdir() if d.is_dir()]
        if available_envs:
            raise FileNotFoundError(
                f"Could not auto-detect environment. Available environments: {available_envs}\n"
                f"Use --build-dir .pio/build/[env_name] to specify manually."
            )
        else:
            raise FileNotFoundError("No build environments found in .pio/build/")

    def detect_flash_files(self, build_dir: str) -> Dict[str, str]:
        """Auto-detect flash files from PlatformIO build directory"""
        build_path = Path(build_dir)
        if not build_path.exists():
            raise FileNotFoundError(f"Build directory not found: {build_dir}")
        
        flash_files = {}
        
        # ESP32-C3 specific files and addresses (different from regular ESP32)
        esp32c3_file_map = {
            'bootloader.bin': '0x0',      # ESP32-C3 bootloader at 0x0
            'partitions.bin': '0x8000',   # Partition table
            'firmware.bin': '0x10000'     # Main application
        }
        
        # Regular ESP32 files and addresses
        esp32_file_map = {
            'bootloader.bin': '0x1000',   # ESP32 bootloader at 0x1000
            'partitions.bin': '0x8000', 
            'boot_app0.bin': '0xe000',    # ESP32 needs boot_app0
            'firmware.bin': '0x10000'
        }
        
        # Try ESP32-C3 layout first
        for filename, address in esp32c3_file_map.items():
            file_path = build_path / filename
            if file_path.exists():
                flash_files[address] = str(file_path)
        
        # If no files found with C3 layout, try regular ESP32 layout
        if not flash_files:
            for filename, address in esp32_file_map.items():
                file_path = build_path / filename
                if file_path.exists():
                    flash_files[address] = str(file_path)
        
        if not flash_files:
            # List available files to help debug
            available_files = [f.name for f in build_path.iterdir() if f.is_file()]
            raise FileNotFoundError(
                f"No flash files found in {build_dir}\n"
                f"Available files: {available_files}\n"
                f"Make sure to run 'pio run' to build the project first."
            )
        
        # Detect chip type from build directory name
        chip_type = "ESP32-C3" if "c3" in build_path.name.lower() else "ESP32"
        print(f"Detected flash files for {chip_type} from {build_path.name}:")
        for addr, path in flash_files.items():
            print(f"  {addr}: {Path(path).name}")
        
        return flash_files
    
    def find_esp32_port(self) -> Optional[str]:
        """Find the first available ESP32 serial port"""
        try:
            available_ports = list(serial.tools.list_ports.comports())
            print(f"Available ports: {[p.device for p in available_ports]}")
            
            # Priority 1: Look for common ESP32 USB-to-serial chips
            esp32_keywords = [
                'cp210', 'cp2102', 'cp2104',  # Silicon Labs
                'ch340', 'ch341',              # WCH
                'ftdi', 'ft232',               # FTDI
                'silicon labs',                # Silicon Labs full name
                'usb-serial',                  # Generic USB serial
                'uart'                         # UART bridges
            ]
            
            for port in available_ports:
                port_desc = port.description.lower()
                port_mfg = (port.manufacturer or '').lower()
                
                print(f"  {port.device}: {port.description} (Manufacturer: {port.manufacturer})")
                
                # Check description and manufacturer for ESP32 chips
                if any(keyword in port_desc or keyword in port_mfg for keyword in esp32_keywords):
                    print(f"Auto-detected ESP32 port: {port.device}")
                    return port.device
            
            # Priority 2: Look for USB ports (exclude built-in serial ports)
            usb_ports = []
            for port in available_ports:
                # Skip built-in serial ports like /dev/ttyS0, /dev/ttyAMA0
                if not any(builtin in port.device for builtin in ['/dev/ttyS', '/dev/ttyAMA', 'COM1', 'COM2']):
                    # Prefer USB-style ports
                    if any(usb_pattern in port.device for usb_pattern in ['/dev/ttyUSB', '/dev/ttyACM', 'COM']):
                        usb_ports.append(port)
            
            if usb_ports:
                selected_port = usb_ports[0].device
                print(f"Auto-detected USB port: {selected_port}")
                if len(usb_ports) > 1:
                    print(f"Note: Multiple USB ports found, using first one")
                    print(f"Available USB ports: {[p.device for p in usb_ports]}")
                return selected_port
            
            # Priority 3: Try PlatformIO device list as fallback
            try:
                result = subprocess.run(['pio', 'device', 'list'], 
                                      capture_output=True, text=True, check=True)
                
                for line in result.stdout.split('\n'):
                    if '/dev/ttyUSB' in line or '/dev/ttyACM' in line:
                        port = line.split()[0]
                        print(f"Auto-detected port via PlatformIO: {port}")
                        return port
                    elif 'COM' in line and 'COM1' not in line and 'COM2' not in line:
                        port_match = re.search(r'COM\d+', line)
                        if port_match:
                            port = port_match.group()
                            print(f"Auto-detected port via PlatformIO: {port}")
                            return port
                            
            except (subprocess.CalledProcessError, FileNotFoundError):
                pass
            
            # If we have any ports at all, show them to help user
            if available_ports:
                print("Could not auto-detect ESP32 port. Available ports:")
                for port in available_ports:
                    print(f"  {port.device}: {port.description}")
                print("Please specify the correct port with --port parameter")
            else:
                print("No serial ports found. Make sure your ESP32 is connected.")
                
        except ImportError:
            print("Warning: pyserial tools not available for port detection")
            print("Please install pyserial: pip install pyserial")
        
        return None
    
    def wait_for_device_connection(self, port: str, timeout: int = 10) -> bool:
        """Wait for a device to be connected to the specified port"""
        print(f"Checking device connection on {port}...")
        
        try:
            # Try to open the port briefly
            with serial.Serial(port, self.baudrate, timeout=1) as ser:
                if ser.is_open:
                    print(f"✓ Device ready on {port}")
                    time.sleep(0.5)  # Brief stabilization
                    # Serial port is automatically closed when exiting the 'with' block
                    return True
        except (serial.SerialException, FileNotFoundError, PermissionError) as e:
            print(f"✗ Cannot connect to {port}: {e}")
            print("Make sure:")
            print("  1. ESP32 device is connected")
            print("  2. No other programs are using the port")
            print("  3. You have permission to access the port")
            return False
        
        return False
    
    def wait_for_device_disconnection(self, port: str, timeout: int = 120):
        """Wait for the device on the specified port to be disconnected."""
        print(f"Flashing complete. Please disconnect the device from port {port}.")
        start_time = time.time()
        while time.time() - start_time < timeout:
            if not self.check_port_ready(port):
                print(f"✓ Device disconnected from {port}. Ready for the next one.")
                time.sleep(1)  # Brief pause before next cycle
                return True
            time.sleep(0.5)
        print(f"✗ Timed out waiting for device on {port} to be disconnected.")
        return False
    
    def erase_flash(self, port: str, chip_type: str = 'auto') -> bool:
        """Completely erase the ESP32 flash memory"""
        try:
            print(f"Erasing flash on {port}...")
            
            # Add a small delay to ensure port is free
            time.sleep(1.5)
            
            cmd = [
                'esptool.py',
                '--port', port,
                '--baud', str(self.baudrate),
                '--chip', chip_type,
                'erase_flash'
            ]
            
            print(f"Running: {' '.join(cmd)}")
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode == 0:
                print(f"✓ Flash erased successfully on {port}")
                print("Erase output:", result.stdout[-200:])  # Show last 200 chars
                return True
            else:
                print(f"✗ Flash erase failed on {port}")
                print("STDOUT:", result.stdout)
                print("STDERR:", result.stderr)
                return False
                
        except subprocess.TimeoutExpired:
            print(f"✗ Flash erase timeout on {port}")
            return False
        except Exception as e:
            print(f"✗ Flash erase error on {port}: {e}")
            return False
    
    def flash_firmware(self, port: str, chip_type: str = 'auto', verify: bool = False) -> bool:
        """Flash all required files to ESP32"""
        try:
            print(f"Flashing firmware to {port}...")
            
            # Add a small delay and verify port exists before flashing
            time.sleep(1.5)
            
            # Check if port still exists
            if not Path(port).exists():
                print(f"✗ Port {port} no longer exists. Device may have disconnected.")
                return False
            
            # Build esptool command with all flash files
            cmd = [
                'esptool.py',
                '--port', port,
                '--baud', str(self.baudrate),
                '--chip', chip_type,
                'write_flash',
                '--flash_mode', 'dio',
                '--flash_freq', '80m',
                '--flash_size', 'detect'
            ]
            
            if verify:
                cmd.append('--verify')
            
            # Add each file with its address
            for address, file_path in self.flash_files.items():
                cmd.extend([address, file_path])
                print(f"  {address}: {Path(file_path).name}")
            
            print(f"Running: {' '.join(cmd[:8])} ...")  # Show command without full paths
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
            
            if result.returncode == 0:
                print(f"✓ Firmware flashed successfully on {port}")
                return True
            else:
                print(f"✗ Firmware flash failed on {port}")
                print("STDOUT:", result.stdout[-500:])  # Last 500 chars
                print("STDERR:", result.stderr[-500:])  # Last 500 chars
                return False
                
        except subprocess.TimeoutExpired:
            print(f"✗ Firmware flash timeout on {port}")
            return False
        except Exception as e:
            print(f"✗ Firmware flash error on {port}: {e}")
            return False
    
    def read_serial_output(self, port: str, timeout: int = 30) -> Optional[Dict]:
        """Read serial output and extract device ID and public key"""
        try:
            print(f"Reading serial output from {port}...")
            
            with serial.Serial(port, self.baudrate, timeout=1) as ser:
                # Reset the device
                ser.setDTR(False)
                time.sleep(0.1)
                ser.setDTR(True)
                time.sleep(0.1)
                ser.setDTR(False)
                time.sleep(0.5)  # Give more time for reset
                
                # Clear any existing data
                ser.flushInput()
                
                output_lines = []
                start_time = time.time()
                device_id = None
                public_key = None
                boot_success = False
                
                while time.time() - start_time < timeout:
                    if ser.in_waiting > 0:
                        line = ser.readline().decode('utf-8', errors='ignore').strip()
                        if line:
                            output_lines.append(line)
                            print(f"[{port}] {line}")
                            
                            # Check for successful boot indicators
                            if any(indicator in line.lower() for indicator in ['app_main', 'hello world', 'setup()', 'ready']):
                                boot_success = True
                            
                            # Check for boot failures
                            if 'invalid header' in line.lower():
                                print(f"⚠️  Boot issue detected: {line}")
                            
                            # Look for serial number (starts with "zap-")
                            if not device_id:
                                serial_patterns = [
                                    r'serial number:\s*(zap-[A-Fa-f0-9]+)',
                                    r'device id:\s*(zap-[A-Fa-f0-9]+)',
                                    r'serial:\s*(zap-[A-Fa-f0-9]+)'
                                ]
                                for pattern in serial_patterns:
                                    serial_match = re.search(pattern, line, re.IGNORECASE)
                                    if serial_match:
                                        device_id = serial_match.group(1)
                                        print(f"✓ Found Serial Number: {device_id}")
                                        break
                            
                            # Look for public key (hex values, typically 64 chars for 32 bytes)
                            if not public_key:
                                key_patterns = [
                                    r'public key:\s*([A-Fa-f0-9]{32,})',
                                    r'pubkey:\s*([A-Fa-f0-9]{32,})',
                                    r'key:\s*([A-Fa-f0-9]{32,})'
                                ]
                                for pattern in key_patterns:
                                    key_match = re.search(pattern, line, re.IGNORECASE)
                                    if key_match:
                                        public_key = key_match.group(1)
                                        print(f"✓ Found Public Key: {public_key}")
                                        break
                            
                            # If we have both, we can break early
                            if device_id and public_key:
                                print("✓ Found both serial number and public key!")
                                break
                    
                    time.sleep(0.1)
                
                # Final status check
                if not boot_success and 'invalid header' in '\n'.join(output_lines):
                    print("⚠️  Device appears to have boot issues - firmware may not be flashed correctly")
                
                return {
                    'port': port,
                    'device_id': device_id,
                    'public_key': public_key,
                    'output_lines': output_lines,
                    'boot_success': boot_success,
                    'timestamp': datetime.now().isoformat()
                }
                
        except Exception as e:
            print(f"✗ Serial read error on {port}: {e}")
            return None
    
    def check_port_ready(self, port: str) -> bool:
        """Quietly check if a port is connected and ready."""
        try:
            # Check if port exists in the list of comports
            if port not in [p.device for p in serial.tools.list_ports.comports()]:
                return False
            # Try to open it briefly to ensure it's responsive
            with serial.Serial(port, self.baudrate, timeout=0.5):
                pass
            return True
        except (serial.SerialException, FileNotFoundError, PermissionError):
            return False

    def process_device(self, port: str, device_number: int, erase_first: bool = True, chip_type: str = 'auto', verify: bool = False) -> Dict:
        """Process a single device: erase, flash, and read output"""
        print(f"\n{'='*60}")
        print(f"PROCESSING DEVICE #{device_number}")
        print(f"{'='*60}")
        
        result = {
            'device_number': device_number,
            'port': port,
            'success': False,
            'serial_number': None,
            'public_key': None,
            'errors': [],
            'warnings': [],
            'timestamp': datetime.now().isoformat()
        }
        
        try:
            # Step 1: Device connection is already confirmed by the main loop.
            
            # Step 2: Erase flash (optional but recommended for clean state)
            if erase_first:
                if not self.erase_flash(port, chip_type):
                    # Don't fail completely if erase fails - continue with flashing
                    warning_msg = 'Flash erase failed, but continuing with flashing (this is normal for fresh devices)'
                    result['warnings'].append(warning_msg)
                    print(f"⚠️  {warning_msg}")
                else:
                    print("✓ Flash erase completed successfully")
            
            # Step 3: Flash firmware (continue regardless of erase result)
            if not self.flash_firmware(port, chip_type, verify):
                result['errors'].append('Firmware flash failed')
                return result
            
            # Step 4: Read serial output
            time.sleep(2)  # Give device time to boot
            serial_data = self.read_serial_output(port)
            
            if serial_data:
                result['serial_number'] = serial_data['device_id']
                result['public_key'] = serial_data['public_key']
                result['output_lines'] = serial_data['output_lines']
                
                if result['serial_number'] and result['public_key']:
                    result['success'] = True
                    print(f"✓ Device #{device_number} completed successfully!")
                    print(f"  Serial: {result['serial_number']}")
                    pub_key_display = f"{result['public_key'][:16]}...{result['public_key'][-8:]}" if len(result['public_key']) > 24 else result['public_key']
                    print(f"  Public Key: {pub_key_display}")
                    if result['warnings']:
                        print(f"  Warnings: {'; '.join(result['warnings'])}")
                else:
                    result['errors'].append('Could not extract serial number or public key')
            else:
                result['errors'].append('Failed to read serial output')
            
        except Exception as e:
            result['errors'].append(f'Unexpected error: {e}')
        
        return result
    
    def run_sequential_flashing(self, port: str, erase_first: bool = True, chip_type: str = 'auto', verify: bool = False, output_file_base: str = None):
        """Run sequential flashing process"""
        print(f"Starting sequential flashing on port: {port}")
        print("This script will automatically detect device connection and disconnection.")
        print("1. Connect an ESP32 device to the port")
        print("2. Wait for flashing to complete")
        print("3. Disconnect the device and connect the next one")
        print("4. Press Ctrl+C to stop when done")
        print()
        
        device_number = 1
        all_results = []
        
        # Determine output file base name at the start
        if not output_file_base:
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            output_file_base = f"flash_results_{timestamp}"
        else:
            # If a base name is provided, append a timestamp to avoid overwriting if run multiple times
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            output_file_base = f"{output_file_base}_{timestamp}"
        
        csv_output_file = f"{output_file_base}.csv"
        print(f"CSV results will be saved to: {csv_output_file}")
        
        try:
            while True:
                print(f"\n--- Waiting for device #{device_number} on {port} ---")
                
                # 1. Wait for connection
                while not self.check_port_ready(port):
                    time.sleep(0.5)
                
                print(f"✓ Device #{device_number} detected on {port}. Starting flash process...")
                time.sleep(1) # Allow port to stabilize before processing

                result = self.process_device(port, device_number, erase_first, chip_type, verify)
                all_results.append(result)
                
                if result['success']:
                    self.append_to_csv(result, csv_output_file)
                else:
                    print(f"\n✗ Device #{device_number} failed. See errors above.")

                # 2. Wait for disconnection, whether it succeeded or failed
                if not self.wait_for_device_disconnection(port):
                    print("Stopping due to timeout waiting for disconnection.")
                    break # Exit the main loop if timeout occurs

                device_number += 1
                
        except KeyboardInterrupt:
            print(f"\n\nFlashing stopped by user after {len(all_results)} devices.")
        
        # Save final JSON summary and show summary
        if all_results:
            self.save_json_results(all_results, output_file_base=output_file_base)
            self.print_summary(all_results)
        
        return all_results
    
    def append_to_csv(self, result: Dict, csv_file_path: str):
        """Append a single successful result to a CSV file."""
        if not result.get('success'):
            return

        csv_headers = ['ecc_serial', 'mac_eth0', 'mac_wlan0', 'helium_public_key', 'full_public_key']
        
        # Check if file exists to determine if we need to write headers
        file_exists = Path(csv_file_path).is_file()

        try:
            with open(csv_file_path, 'a', newline='') as csvfile:
                writer = csv.DictWriter(csvfile, fieldnames=csv_headers)
                if not file_exists:
                    writer.writeheader()
                
                ecc_serial = result.get('serial_number', '')
                full_public_key = result.get('public_key', '')
                
                writer.writerow({
                    'ecc_serial': ecc_serial,
                    'mac_eth0': '',  # Empty as per requirement
                    'mac_wlan0': '',  # Empty as per requirement
                    'helium_public_key': '',  # Empty as per requirement
                    'full_public_key': full_public_key
                })
            print(f"Result for device #{result.get('device_number')} appended to: {csv_file_path}")
        except IOError as e:
            print(f"Error writing to CSV file {csv_file_path}: {e}")
        except Exception as e:
            print(f"An unexpected error occurred while writing CSV: {e}")

    def save_json_results(self, results: List[Dict], output_file_base: str):
        """Save final results to a JSON file."""
        # Save JSON results
        json_output_file = f"{output_file_base}.json"
        with open(json_output_file, 'w') as f:
            json.dump(results, f, indent=2)
        print(f"\nJSON results saved to: {json_output_file}")

    def print_summary(self, results: List[Dict]):
        """Print summary of results"""
        successful = sum(1 for r in results if r['success'])
        total = len(results)
        
        print(f"\n{'='*60}")
        print(f"SEQUENTIAL FLASH OPERATION SUMMARY")
        print(f"{'='*60}")
        print(f"Total devices processed: {total}")
        print(f"Successful: {successful}")
        print(f"Failed: {total - successful}")
        print(f"Success rate: {successful/total*100:.1f}%" if total > 0 else "No devices processed")
        
        if successful > 0:
            print(f"\nSUCCESSFUL DEVICES:")
            for result in results:
                if result['success']:
                    serial_num = result['serial_number']
                    pub_key = result['public_key']
                    key_display = f"{pub_key[:16]}...{pub_key[-8:]}" if len(pub_key) > 24 else pub_key
                    print(f"  Device #{result['device_number']}: {serial_num} | Key: {key_display}")
        
        failed_devices = [r for r in results if not r['success']]
        if failed_devices:
            print(f"\nFAILED DEVICES:")
            for result in failed_devices:
                print(f"  Device #{result['device_number']}: {', '.join(result['errors'])}")


def list_available_ports():
    """List all available serial ports for debugging"""
    try:
        available_ports = list(serial.tools.list_ports.comports())
        
        print("=== AVAILABLE SERIAL PORTS ===")
        if not available_ports:
            print("No serial ports found!")
            return
            
        for i, port in enumerate(available_ports, 1):
            print(f"{i}. {port.device}")
            print(f"   Description: {port.description}")
            print(f"   Manufacturer: {port.manufacturer}")
            print(f"   VID:PID: {port.vid}:{port.pid}" if port.vid and port.pid else "   VID:PID: Unknown")
            print()
            
    except ImportError:
        print("Error: pyserial not installed. Run: pip install pyserial")


def debug_flash_setup(build_dir: str = None):
    """Debug function to check flash setup"""
    print("=== FLASH SETUP DEBUG ===")
    
    try:
        flasher = ESP32SequentialFlasher(build_dir=build_dir)
        
        print(f"Flash files detected: {len(flasher.flash_files)}")
        for addr, path in flasher.flash_files.items():
            file_path = Path(path)
            size = file_path.stat().st_size if file_path.exists() else 0
            print(f"  {addr}: {file_path.name} ({size} bytes)")
            if not file_path.exists():
                print(f"    ❌ FILE NOT FOUND: {path}")
            elif size == 0:
                print(f"    ❌ FILE IS EMPTY")
            else:
                print(f"    ✅ File OK")
        
        print("\nTesting esptool availability:")
        try:
            result = subprocess.run(['esptool.py', '--help'], 
                                  capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                print("✅ esptool.py is available")
            else:
                print("❌ esptool.py not working properly")
        except FileNotFoundError:
            print("❌ esptool.py not found - install with: pip install esptool")
        except subprocess.TimeoutExpired:
            print("❌ esptool.py timeout")
            
    except Exception as e:
        print(f"❌ Error setting up flasher: {e}")


def test_device_connection(port: str):
    """Test basic device connection"""
    print(f"=== TESTING CONNECTION TO {port} ===")
    
    try:
        # Test 1: Basic chip detection
        print("1. Testing chip detection...")
        cmd = ['esptool.py', '--port', port, 'chip_id']
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        
        if result.returncode == 0:
            print("✅ Chip detection successful")
            print("Output:", result.stdout[-300:])
        else:
            print("❌ Chip detection failed")
            print("STDERR:", result.stderr)
            return False
            
        # Test 2: Flash info
        print("\n2. Reading flash info...")
        cmd = ['esptool.py', '--port', port, 'flash_id']
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        
        if result.returncode == 0:
            print("✅ Flash info read successful")
            print("Output:", result.stdout[-300:])
        else:
            print("❌ Flash info failed")
            print("STDERR:", result.stderr)
            
        return True
        
    except subprocess.TimeoutExpired:
        print("❌ Connection test timeout")
        return False
    except Exception as e:
        print(f"❌ Connection test error: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(description='ESP32 Sequential Flashing Tool')
    
    # Either specify build directory or individual files
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--build-dir', help='PlatformIO build directory (auto-detects files)')
    group.add_argument('--files', nargs='+', help='Manual flash files in format addr:file (e.g., 0x1000:bootloader.bin 0x10000:firmware.bin)')
    
    parser.add_argument('--port', help='Serial port (auto-detect if not specified)')
    parser.add_argument('--verify-flash', action='store_true', help='Verify flash after writing')
    parser.add_argument('--chip', help='Specify chip type (esp32, esp32c3, esp32s2, esp32s3)')
    parser.add_argument('--list-ports', action='store_true', help='List available serial ports and exit')
    parser.add_argument('--debug', action='store_true', help='Debug mode - check setup without flashing')
    parser.add_argument('--test-connection', action='store_true', help='Test device connection only')
    parser.add_argument('--baudrate', type=int, default=115200, help='Serial baudrate')
    parser.add_argument('--no-erase', action='store_true', help='Skip flash erase step')
    parser.add_argument('--timeout', type=int, default=30, help='Serial read timeout in seconds')
    parser.add_argument('--output-base', help='Base name for output files (e.g., my_flash_run). A timestamp will be appended.')
    
    args = parser.parse_args()
    
    # Handle debug modes
    if args.debug:
        debug_flash_setup(args.build_dir)
        sys.exit(0)
        
    if args.test_connection:
        if not args.port:
            print("Error: --port required for connection test")
            sys.exit(1)
        success = test_device_connection(args.port)
        sys.exit(0 if success else 1)
    
    # Handle port listing
    if args.list_ports:
        list_available_ports()
        sys.exit(0)
    
    try:
        # Parse flash files
        if args.files:
            # Parse manual files format: addr:file
            flash_files = {}
            for file_spec in args.files:
                if ':' not in file_spec:
                    print(f"Error: Invalid file format '{file_spec}'. Use addr:file format (e.g., 0x1000:bootloader.bin)")
                    sys.exit(1)
                addr, filepath = file_spec.split(':', 1)
                flash_files[addr] = filepath
            
            flasher = ESP32SequentialFlasher(flash_files=flash_files, baudrate=args.baudrate)
        else:
            # Use build directory (or auto-detect)
            flasher = ESP32SequentialFlasher(build_dir=args.build_dir, baudrate=args.baudrate)
        
        # Get port
        if args.port:
            port = args.port
            print(f"Using specified port: {port}")
        else:
            port = flasher.find_esp32_port()
            if not port:
                print("No port detected. Please specify port manually with --port")
                sys.exit(1)
        
        # Run sequential flashing
        chip_type = args.chip or 'auto'
        flasher.run_sequential_flashing(port, not args.no_erase, chip_type, args.verify_flash, args.output_base)
        
    except KeyboardInterrupt:
        print("\nOperation cancelled by user.")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()