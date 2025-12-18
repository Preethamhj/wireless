
export default function Navbar({ active, setActive }) {
  return (
    <nav className="flex flex-col md:flex-row items-center justify-between px-8 py-4 bg-gradient-to-r from-gray-900 via-gray-800 to-gray-900 shadow-lg border-b-2 border-green-500">
      {/* Left Section */}
      <div className="flex items-center gap-8 w-full md:w-auto justify-between md:justify-start">
        <div className="text-2xl font-extrabold text-green-400 tracking-wider drop-shadow-lg select-none">
          ESP32 Twin
        </div>
        <div className="hidden md:flex gap-6 ml-8">
          {["Dashboard", "Code Editor"].map(tab => (
            <button
              key={tab}
              onClick={() => setActive(tab)}
              className={`transition-all px-3 py-1 rounded-lg font-semibold text-lg ${
                active === tab
                  ? "bg-green-500 text-white shadow border-b-4 border-green-700"
                  : "text-green-200 hover:bg-gray-700 hover:text-green-300"
              }`}
            >
              {tab}
            </button>
          ))}
        </div>
      </div>
      {/* Right Section */}
      <div className="flex gap-4 mt-3 md:mt-0">
        {/* Placeholder for user profile, settings, or actions */}
        <button className="bg-gray-700 hover:bg-green-600 text-green-200 px-4 py-1 rounded-lg font-bold transition-all">Login</button>
        <button className="bg-green-600 hover:bg-green-700 text-white px-4 py-1 rounded-lg font-bold transition-all">Sign Up</button>
      </div>
      {/* Mobile Nav */}
      <div className="flex md:hidden w-full mt-3 gap-4 justify-center">
        {["Dashboard", "Code Editor"].map(tab => (
          <button
            key={tab}
            onClick={() => setActive(tab)}
            className={`transition-all px-3 py-1 rounded-lg font-semibold text-lg ${
              active === tab
                ? "bg-green-500 text-white shadow border-b-4 border-green-700"
                : "text-green-200 hover:bg-gray-700 hover:text-green-300"
            }`}
          >
            {tab}
          </button>
        ))}
      </div>
    </nav>
  );
}
