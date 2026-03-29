import { BleManager, Device, Characteristic } from 'react-native-ble-plx';
import { useEffect, useState } from 'react';
import { Buffer } from 'buffer';

const manager = new BleManager();

export function useBLE() {
  const [device, setDevice] = useState<Device | null>(null);
  const [isConnected, setIsConnected] = useState(false);

  // Scan for devices
  const scanForDevice = () => {
    manager.startDeviceScan(null, null, (error, scannedDevice: Device | null) => {
      if (error) { console.error(error); return; }
      if (scannedDevice != null) {
        if (scannedDevice.name === 'nrf54_strength_training') {
          manager.stopDeviceScan();
          connectToDevice(scannedDevice);
        }
      }
    });
  };

  // Connect
  const connectToDevice = async (scannedDevice: Device) => {
    try {
      const connected = await scannedDevice.connect();
      await connected.discoverAllServicesAndCharacteristics();
      setDevice(connected);
      setIsConnected(true);
    } catch (e) {
      console.error('Connection failed:', e);
    }
  };

  // Read / Write a characteristic
  const readCharacteristic = async (serviceUUID: string, charUUID: string) => {
    if (!device) throw new Error('No device connected');
    const char = await device.readCharacteristicForService(serviceUUID, charUUID);
    return Buffer.from(char.value ?? '', 'base64');
  };

  const writeCharacteristic = async (serviceUUID: string, charUUID: string, data: Uint8Array) => {
    if (!device) throw new Error('No device connected');
    const base64 = Buffer.from(data).toString('base64');
    await device.writeCharacteristicWithResponseForService(serviceUUID, charUUID, base64);
  };

  // Subscribe to notifications
  const subscribeToNotifications = (
    serviceUUID: string,
    charUUID: string,
    callback: (data: Buffer) => void
  ) => {
    if (!device) throw new Error('No device connected');
    device.monitorCharacteristicForService(serviceUUID, charUUID, (error: Error | null, char: Characteristic | null) => {
      if (error || !char?.value) return;
      const decoded = Buffer.from(char.value, 'base64');
      callback(decoded);
    });
  };

  // Cleanup
  useEffect(() => {
    return () => { manager.destroy(); };
  }, []);

  return { 
    scanForDevice, 
    readCharacteristic, 
    writeCharacteristic, 
    subscribeToNotifications, 
    isConnected,
    connectedDeviceName: device?.name ?? null,
  };
}