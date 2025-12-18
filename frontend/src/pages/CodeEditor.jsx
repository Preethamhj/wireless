
import React, { useState } from "react";
import EditorPanel from "../components/EditorPanel";
import Terminal from "../components/Terminal";

export default function CodeEditor() {
  const [logs, setLogs] = useState("");
  const [buildId, setBuildId] = useState(null);
  return (
    <div className="flex flex-col md:flex-row h-[calc(100vh-80px)] p-4 gap-4">
      <div className="flex-1 md:w-1/2 h-1/2 md:h-full">
        <EditorPanel onLogsChange={setLogs} onBuildIdChange={setBuildId} />
      </div>
      <div className="flex-1 md:w-1/2 h-1/2 md:h-full flex flex-col">
        <Terminal logs={logs} buildId={buildId} />
      </div>
    </div>
  );
}
