import { compileFirmware, assignOTA } from "../api/backendApi";

const DEVICE_ID = "esp32_01";

const handleCompile = async () => {
  const res = await compileFirmware(code);
  setLogs(res.logs || "Compile done");
  setBuildId(res.build_id);
};

const handleUpload = async () => {
  if (!buildId) {
    setLogs("Compile first!");
    return;
  }
  await assignOTA(DEVICE_ID, buildId);
  setLogs("OTA assigned. Device will poll shortly.");
};
export default {
  handleCompile,
  handleUpload
}
