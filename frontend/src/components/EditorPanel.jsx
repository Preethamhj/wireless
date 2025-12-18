import React, { useState } from "react";
import Editor from "@monaco-editor/react";
import { compileFirmware, assignOTA } from "../api/backendApi";
import { DEFAULT_FIRMWARE } from "../constants/defaultFirmware";

const DEVICE_ID = "esp32_01";

export default function EditorPanel({ onLogsChange }) {
  const [code, setCode] = useState(DEFAULT_FIRMWARE);
  const [loading, setLoading] = useState(false);
  const [buildId, setBuildId] = useState(null);

  const handleCompile = async () => {
    if (!code || code.trim() === "") {
      onLogsChange?.("No firmware code provided");
      return;
    }

    try {
      setLoading(true);
      onLogsChange?.("Compiling firmware...");

      const res = await compileFirmware(code);

      if (!res.success) {
        onLogsChange?.(res.logs || "Compilation failed");
        return;
      }

      setBuildId(res.build_id);
      onLogsChange?.(res.logs || "Compilation successful");

    } catch (err) {
      onLogsChange?.(
        err?.response?.data?.logs ||
        err.message ||
        "Compile failed"
      );
    } finally {
      setLoading(false);
    }
  };

  const handleUpload = async () => {
    if (!buildId) {
      onLogsChange?.("Compile firmware first!");
      return;
    }

    try {
      setLoading(true);
      onLogsChange?.("Assigning OTA update...");

      await assignOTA(DEVICE_ID, buildId);

      onLogsChange?.("OTA assigned successfully. Device will poll shortly.");
    } catch (err) {
      onLogsChange?.(
        err?.response?.data?.error ||
        err.message ||
        "OTA assignment failed"
      );
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="h-full w-full bg-gray-900 rounded-lg shadow-lg border border-green-700 flex flex-col">
      <div className="px-4 py-2 bg-gray-800 border-b border-green-700 text-green-400 font-semibold rounded-t-lg">
        Firmware Code Editor
      </div>

      <div className="flex-1 min-h-[350px] h-[50vh] w-full">
        <Editor
          height="100%"
          width="100%"
          language="cpp"
          theme="vs-dark"
          value={code}
          onChange={(value) => setCode(value || "")}
          options={{
            fontSize: 16,
            minimap: { enabled: false },
            wordWrap: "on",
            scrollBeyondLastLine: false,
            automaticLayout: true,
          }}
        />
      </div>

      <div className="flex gap-3 mt-4 px-4 pb-4">
        <button
          onClick={handleCompile}
          disabled={loading}
          className="px-4 py-2 bg-blue-600 text-white rounded shadow hover:bg-blue-700 disabled:opacity-60"
        >
          Compile
        </button>

        <button
          onClick={handleUpload}
          disabled={loading || !buildId}
          className="px-4 py-2 bg-green-600 text-white rounded shadow hover:bg-green-700 disabled:opacity-60"
        >
          Upload OTA
        </button>
      </div>
    </div>
  );
}
