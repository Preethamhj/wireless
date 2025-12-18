import { useEffect, useState } from "react";

export default function Terminal({ logs, buildId }) {
  const [otaEvents, setOtaEvents] = useState([]);

  // Poll backend for OTA fallback events
  useEffect(() => {
    const interval = setInterval(async () => {
      try {
        const res = await fetch("http://localhost:8000/ota/events");
        const data = await res.json();
        setOtaEvents(data);
      } catch (err) {
        // silent fail (backend may be down)
      }
    }, 2000);

    return () => clearInterval(interval);
  }, []);

  return (
    <div className="bg-black p-4 rounded-lg border border-green-700 text-green-400 font-mono h-full flex flex-col">
      <h3 className="mb-2 text-green-300 font-bold">Logs</h3>

      <div className="flex-1 overflow-y-auto whitespace-pre-wrap text-sm space-y-1">
        
        {/* Existing logs */}
        {logs ? (
          <div>{logs}</div>
        ) : (
          <span className="text-gray-500">&gt; Waiting for actions...</span>
        )}

        {/* OTA fallback events */}
        {otaEvents.length > 0 && (
          <>
            <div className="text-green-500 mt-2">--- OTA EVENTS ---</div>
            {otaEvents.map((e, i) => (
              <div key={i}>
                [{e.timestamp}] OTA FALLBACK → device={e.device_id} build={e.build_id} status={e.status} reason={e.reason}
              </div>
            ))}
          </>
        )}
      </div>

      {buildId && (
        <div className="mt-2 text-xs text-gray-400">
          Build ID: <b>{buildId}</b>
        </div>
      )}
    </div>
  );
}
