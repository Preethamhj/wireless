import DeviceStatus from "../components/DeviceStatus";
import DigitalTwin from "../components/DigitalTwin";

export default function Dashboard() {
  return (
    <div className="p-6 grid grid-cols-1 md:grid-cols-2 gap-6">
      <DeviceStatus />
      <DigitalTwin />
    </div>
  );
}
