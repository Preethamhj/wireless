
export default function Terminal({ logs, buildId }) {
  return (
    <div className="bg-black p-4 rounded-lg border border-green-700 text-green-400 font-mono h-full flex flex-col">
      <h3 className="mb-2 text-green-300 font-bold">Logs</h3>
      <div className="flex-1 overflow-y-auto whitespace-pre-wrap text-sm">
        {logs ? logs : <span className="text-gray-500">&gt; Waiting for actions...</span>}
      </div>
      {buildId && (
        <div className="mt-2 text-xs text-gray-400">Build ID: <b>{buildId}</b></div>
      )}
    </div>
  );
}
