import React, { createContext, useCallback, useContext, useEffect, useRef, useState } from 'react';
import { BleManager, Device, Characteristic } from 'react-native-ble-plx';
import { Buffer } from 'buffer';

interface BLEContextType {
  scanForDevice: () => void;
  isConnected: boolean;
  connectedDeviceName: string | null;
  readCharacteristic: (svc: string, char: string) => Promise<Buffer>;
  writeCharacteristic: (svc: string, char: string, data: Uint8Array) => Promise<void>;
  subscribeToNotifications: (
    svc: string,
    char: string,
    callback: (data: Buffer) => void
  ) => { remove: () => void } | undefined;
}

const BLEContext = createContext<BLEContextType | null>(null);

export const BLEProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const manager = useRef(new BleManager()).current;
  const [device, setDevice] = useState<Device | null>(null);
  const [isConnected, setIsConnected] = useState(false);

  // useCallback on every exposed function so the hook call order
  // is always identical between renders — fixes "change in order of hooks" error
  const connectToDevice = useCallback(async (scannedDevice: Device) => {
    try {
      const connected = await scannedDevice.connect();
      await connected.discoverAllServicesAndCharacteristics();
      setDevice(connected);
      setIsConnected(true);

      connected.onDisconnected(() => {
        setDevice(null);
        setIsConnected(false);
      });
    } catch (e) {
      console.error('Connection failed:', e);
    }
  }, []); // manager is a ref — stable forever, no deps needed

  const scanForDevice = useCallback(() => {
    if (isConnected) return;
    manager.startDeviceScan(null, null, (error, scannedDevice) => {
      if (error) { console.error('Scan error:', error); return; }
      if (scannedDevice?.name === 'nrf54_strength_training') {
        manager.stopDeviceScan();
        connectToDevice(scannedDevice);
      }
    });
  }, [isConnected, connectToDevice]);

  const readCharacteristic = useCallback(async (svc: string, char: string): Promise<Buffer> => {
    if (!device) throw new Error('No device connected');
    const c = await device.readCharacteristicForService(svc, char);
    return Buffer.from(c.value ?? '', 'base64');
  }, [device]);

  const writeCharacteristic = useCallback(async (
    svc: string,
    char: string,
    data: Uint8Array
  ): Promise<void> => {
    if (!device) throw new Error('No device connected');
    const base64 = Buffer.from(data).toString('base64');
    await device.writeCharacteristicWithResponseForService(svc, char, base64);
  }, [device]);

  const subscribeToNotifications = useCallback((
    svc: string,
    char: string,
    callback: (data: Buffer) => void
  ) => {
    if (!device) {
      console.warn('subscribeToNotifications: no device connected');
      return undefined;
    }
    console.log('Subscribing to', svc, char);
    const sub = device.monitorCharacteristicForService(
      svc, char,
      (error: Error | null, c: Characteristic | null) => {
        if (error) {
          console.error('BLE notification error:', error.message);
          return;
        }
        if (!c?.value) return;
        callback(Buffer.from(c.value, 'base64'));
      }
    );
    return { remove: () => sub.remove() };
  }, [device]);

  useEffect(() => {
    return () => { manager.destroy(); };
  }, []);

  return (
    <BLEContext.Provider value={{
      scanForDevice,
      isConnected,
      connectedDeviceName: device?.name ?? null,
      readCharacteristic,
      writeCharacteristic,
      subscribeToNotifications,
    }}>
      {children}
    </BLEContext.Provider>
  );
};

export const useBLE = () => {
  const ctx = useContext(BLEContext);
  if (!ctx) throw new Error('useBLE must be used inside BLEProvider');
  return ctx;
};