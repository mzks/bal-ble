# central handling script
#

import argparse
from datetime import datetime

import asyncio
import sys
from itertools import count, takewhile
from typing import Iterator

from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic
from bleak.backends.device import BLEDevice
from bleak.backends.scanner import AdvertisementData

UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
UART_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
UART_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


# TIP: you can get this function and more from the ``more-itertools`` package.
def sliced(data: bytes, n: int) -> Iterator[bytes]:
    return takewhile(len, (data[i : i + n] for i in count(0, n)))


async def uart_terminal(args=None):

    def match_nus_uuid(device: BLEDevice, adv: AdvertisementData):
        # This assumes that the device includes the UART service UUID in the
        # advertising data. This test may need to be adjusted depending on the
        # actual advertising data supplied by the device.
        if UART_SERVICE_UUID.lower() in adv.service_uuids:
            return True

        return False

    device = await BleakScanner.find_device_by_filter(match_nus_uuid)

    if device is None:
        print("no matching device found, you may need to UUID.")
        sys.exit(1)

    def handle_disconnect(_: BleakClient):
        print("Device was disconnected, goodbye.")
        for task in asyncio.all_tasks():
            task.cancel()

    def handle_rx(_: BleakGATTCharacteristic, data: bytearray):

        if args.filename:
            with open(args.filename, mode='a') as f:
                if args.time:
                    f.write(datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f, "))
                f.write(data)

        if args.echo:
            print("Rx:", data)

    async with BleakClient(device, disconnected_callback=handle_disconnect) as client:
        await client.start_notify(UART_TX_CHAR_UUID, handle_rx)

        print("Connected, start typing command (if defined) and press ENTER...")

        loop = asyncio.get_running_loop()
        nus = client.services.get_service(UART_SERVICE_UUID)
        rx_char = nus.get_characteristic(UART_RX_CHAR_UUID)

        while True:
            data = await loop.run_in_executor(None, sys.stdin.buffer.readline)

            # data will be empty on EOF (e.g. CTRL+D on *nix)
            if not data:
                break

            for s in sliced(data, rx_char.max_write_without_response_size):
                await client.write_gatt_char(rx_char, s, response=False)

            print("sent:", data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
            prog='central',
            description='bal-ble central script')
    parser.add_argument('-f', '--filename', default=None, help='Filename to save the recieved data')
    parser.add_argument('-e', '--echo', action='store_true', help='Show recieved data on terminal')
    parser.add_argument('-t', '--time', action='store_true', help='Append datetime for the recieved data')
    args = parser.parse_args()

    try:
        asyncio.run(uart_terminal(args))
    except asyncio.CancelledError:
        # task is cancelled on disconnect, so we ignore this error
        pass
