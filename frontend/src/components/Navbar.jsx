export default function Navbar({ active, setActive }) {
  return (
    <nav className="flex items-center justify-between px-6 py-3 bg-gray-800 border-b border-green-600">
      <div className="text-xl font-bold text-green-400">
        ESP32 Twin
      </div>

      <div className="flex gap-8">
        {["Dashboard", "Code Editor"].map(tab => (
          <button
            key={tab}
            onClick={() => setActive(tab)}
            className={`font-medium ${
              active === tab
                ? "text-green-400 border-b-2 border-green-400"
                : "text-gray-300"
            }`}
          >
            {tab}
          </button>
        ))}
      </div>

      <div />
    </nav>
  );
}
