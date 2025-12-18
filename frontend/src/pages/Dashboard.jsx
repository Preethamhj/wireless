import DeviceStatus from "../components/DeviceStatus";
import DigitalTwin from "../components/DigitalTwin";

export default function Dashboard() {
  return (
    <div className="p-6 min-h-[calc(100vh-80px)] flex flex-col gap-6">

      {/* Device Status – small & centered under navbar */}
      <div className="flex justify-center">
        <div className="w-full max-w-md">
          <DeviceStatus />
        </div>
      </div>

      {/* Digital Twin – takes ~60% height */}
      <div className="flex-1">
        <DigitalTwin />
      </div>

    </div>
  );
}
