export const getDeviceStatus = () =>
  new Promise((resolve) => {
    setTimeout(() => {
      resolve({
        online: true,
        wifiConfirmed: false,
        lastSeen: "2 seconds ago"
      });
    }, 800);
  });

export const compileCode = () =>
  new Promise((resolve) => {
    setTimeout(() => {
      resolve({
        success: true,
        logs: "Compilation successful ✔"
      });
    }, 1000);
  });

export const uploadFirmware = () =>
  new Promise((resolve) => {
    setTimeout(() => {
      resolve({
        success: true,
        logs: "OTA triggered. Device will update shortly."
      });
    }, 1200);
  });
