# Copyleft Whiterose from the Dark Army
#
# This file is part of PortaPack.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

# this tool allow you flash pp and copy the apps in fastest way. note that you need to have a sdcard switcher to switch sdcard to pc.
# although, if you don't have one, you still can take out the sdcard and plug it into your pc, but i think it would not pretty fast.

#!/usr/bin/env python3

import sys
import glob
import serial
import serial.tools.list_ports
import os
import shutil
import time
import subprocess
import tempfile

# config
SUDO_PASSWORD = ""
SDCARD_LABEL = "portapack"


def write_command(ser, command, wait_response=None, remove_echo=False, dont_wait_ch=False):
    
    # clean buffer
    while True:
        try:
            if ser.in_waiting:
                ser.read(ser.in_waiting)
            else:
                break
        except:
            break
    
    command_with_newline = command + "\r\n"
    try:
        ser.write(command_with_newline.encode())
    except Exception as e:
        return f"write err: {str(e)}"
    
    response = ""
    expected_echo = f"{command}\r\n"
    wait_response_met = wait_response is None
    
    while True:
        try:
            if ser.in_waiting:
                msg = ser.read(ser.in_waiting).decode('utf-8')
                response += msg
                
                if wait_response and wait_response in response:
                    wait_response_met = True
                
                if wait_response_met and (dont_wait_ch or "ch> " in response):
                    if remove_echo:
                        return response.replace(expected_echo, "")
                    return response
                    
        except serial.SerialTimeoutException:
            continue
        except Exception as e:
            return f"e: {str(e)}"

def get_serial_devices():
    ports = serial.tools.list_ports.comports()
    
    if not ports:
        print("no serial device found")
        return None
    
    portapack_device = None
    for port in ports:
        if port.product == "PortaPack Mayhem":
            print(f"found your pp:")
            print(f"device: {port.device}")
            print(f"description: {port.description}")
            print(f"hwid: {port.hwid}")
            if port.manufacturer:
                print(f"manufacturer: {port.manufacturer}")
            if port.serial_number:
                print(f"serial_number: {port.serial_number}")
            portapack_device = port
            break
    
    if not portapack_device:
        print("no pp device found")
    
    return portapack_device

def run_sudo_command(cmd):

    if not SUDO_PASSWORD:
        print("sudo password not set, please hard code it in the script")
        return
    
    try:
        process = subprocess.Popen(
            ['sudo', '-S'] + cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )
        stdout, stderr = process.communicate(input=SUDO_PASSWORD + '\n')
        if process.returncode != 0:
            raise subprocess.CalledProcessError(process.returncode, cmd, stdout, stderr)
        return stdout
    except subprocess.CalledProcessError as e:
        print(f"sudo command failed: {e}")
        raise

def get_pp_device_linux():
    try:
        cmd = ["lsblk", "-o", "NAME,LABEL,FSTYPE,SIZE,MOUNTPOINT", "-n", "-p"]
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        # find your pp sd
        for line in result.stdout.splitlines():
            clean_line = line.replace('└─', '').replace('├─', '').strip()
            parts = clean_line.split()
            
            if len(parts) >= 2 and SDCARD_LABEL in parts[1]: # checker
                device_path = parts[0]
                # if path valid
                if not os.path.exists(device_path):
                    continue
                    
                # if mounted, return mountpoint
                if len(parts) >= 5:
                    mountpoint = parts[4]
                    return device_path, mountpoint
                
                # if not, create temp dir and mount
                mount_point = tempfile.mkdtemp()
                print(f"mount {device_path} to {mount_point}")
                run_sudo_command(["mount", "-o", f"uid={os.getuid()},gid={os.getgid()}", device_path, mount_point])
                return device_path, mount_point
                
        return None, None
    except subprocess.CalledProcessError as e:
        print(f"command failed: {str(e)}")
        return None, None

def wait_for_pp_drive():
    while True:
        if os.name == 'nt':  # win didn't test yet, TODO
            drives = [d + ':' for d in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' if os.path.exists(d + ':')]
            for drive in drives:
                if os.path.basename(os.path.abspath(drive)).lower() == SDCARD_LABEL:
                    return drive, None
        else:  # linux and mac
            device_path, mount_point = get_pp_device_linux()
            if device_path and mount_point:
                return mount_point, device_path
        time.sleep(1)

def run():
    # worker 1: check device connected
    device = get_serial_devices()
    if not device:
        print("no pp device found")
        return
    
    try:
        # worker 2: open serial and send hackrf cmd
        ser = serial.Serial(device.device, baudrate=115200, timeout=1)
        for i in range(2):
            write_command(ser, "hackrf")
        ser.close()

        # worker 3: hint user to swtich sdcard to pc
        print("\033[92m\n\t+------------------------+")
        print("\t| switch PP sdcard to PC |")
        print("\t+------------------------+\033[0m")
        print("wait PP sdcard...")
        
        # worker 4: wait pp sd
        mount_point, device_path = wait_for_pp_drive()
        if not mount_point:
            print("no pp sd found")
            return
        print(f"find pp sd: {mount_point}")

        # worker 5: copy ppma and ppmp apps
        apps_dir = os.path.join(mount_point, 'APPS')
        if not os.path.exists(apps_dir):
            os.makedirs(apps_dir)

        firmware_dir = os.path.join('.', 'firmware', 'application')
        for ext in ['ppma', 'ppmp']:
            for file in glob.glob(os.path.join(firmware_dir, f'*.{ext}')):
                dest = os.path.join(apps_dir, os.path.basename(file))
                print(f"cp: {os.path.basename(file)}")
                shutil.copy2(file, dest)
        
        print("apps copy done")

        # worker 6: umount pp sd
        if os.name != 'nt' and device_path:
            run_sudo_command(["umount", mount_point])
            os.rmdir(mount_point)  # 删除临时挂载点

        # worker 7: run flash cmd with hackrf_spiflash util
        print("\nflash...")
        process = subprocess.Popen(
            ["hackrf_spiflash", "-w", "./firmware/portapack-mayhem-firmware.bin"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True
        )
        
        while True:
            output = process.stdout.readline()
            if output:
                print(output.strip())
            if process.poll() is not None:
                break
                
        if process.returncode != 0:
            raise subprocess.CalledProcessError(process.returncode, "hackrf_spiflash")

        # worker 8: hint user to swtich sdcard switcher back to pp
        print("\033[92m\n\t+------------------------------------------------+")
        print("\t| switch sdcard switcher back to pp, then reboot |")
        print("\t+------------------------------------------------+\033[0m")

    except serial.SerialException as e:
        print(f"serial comm error: {str(e)}")
    except OSError as e:
        print(f"file operation error: {str(e)}")
    except subprocess.CalledProcessError as e:
        print(f"system command error: {str(e)}")
    except Exception as e:
        print(f"error: {str(e)}")
        # ensure umount when error
        if os.name != 'nt' and 'mount_point' in locals() and device_path:
            try:
                run_sudo_command(["umount", mount_point])
                os.rmdir(mount_point)
            except:
                pass

if __name__ == "__main__":
    try:
        run()
    except Exception as e:
        print(f"error: {str(e)}")
        sys.exit(1)
