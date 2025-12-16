import { createContext, useContext, useState } from "react";

const DeviceContext = createContext();

export function DeviceProvider({ children }) {
  const [deviceStatus, setDeviceStatus] = useState({
    online: false,
    lastSeen: null,
    networkConfirmed: false
  });

  return (
    <DeviceContext.Provider value={{ deviceStatus, setDeviceStatus }}>
      {children}
    </DeviceContext.Provider>
  );
}

export function useDevice() {
  return useContext(DeviceContext);
}
