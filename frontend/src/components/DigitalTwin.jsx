import { Canvas } from "@react-three/fiber";
import { OrbitControls, PerspectiveCamera } from "@react-three/drei";
import { useEffect, useState } from "react";

/* =========================
   40 PINS IN 5x8 GRID
   ========================= */
const DEFAULT_PINS = Array.from({ length: 40 }, (_, i) => ({
  pin: `GPIO${i}`,
  used: false,
  working: false
}));

/* =========================
   COLOR LOGIC
   ========================= */
function getPinColor(pin) {
  if (!pin.used) return "bg-gray-400";
  if (pin.working) return "bg-green-500";
  return "bg-red-500";
}

function getThreeColor(pin) {
  if (!pin.used) return "#9ca3af";
  if (pin.working) return "#22c55e";
  return "#ef4444";
}

/* =========================
   ACCURATE ESP32 MODEL
   ========================= */
function ESP32Model({ pins }) {
  const leftPins = pins.slice(0, 15);  // 15 pins on left
  const rightPins = pins.slice(15, 30); // 15 pins on right
  
  return (
    <group rotation={[0, 0, 0]}>
      {/* Main PCB Board - Dark Blue/Black */}
      <mesh position={[0, 0, 0]} castShadow receiveShadow>
        <boxGeometry args={[2.5, 0.1, 5]} />
        <meshStandardMaterial color="#0f1419" roughness={0.8} />
      </mesh>
      
      {/* White silkscreen border */}
      <mesh position={[0, 0.055, 0]}>
        <boxGeometry args={[2.52, 0.001, 5.02]} />
        <meshStandardMaterial color="#ffffff" transparent opacity={0.3} />
      </mesh>
      
      {/* Large Metal Shield (ESP32-WROOM module) */}
      <mesh position={[0, 0.2, 0.3]} castShadow>
        <boxGeometry args={[1.8, 0.3, 2.5]} />
        <meshStandardMaterial 
          color="#b8b8b8" 
          metalness={0.9} 
          roughness={0.15}
        />
      </mesh>
      
      {/* Shield label area (darker top) */}
      <mesh position={[0, 0.36, 0.3]}>
        <boxGeometry args={[1.75, 0.02, 2.45]} />
        <meshStandardMaterial color="#404040" metalness={0.3} roughness={0.7} />
      </mesh>
      
      {/* ESP32 text simulation on shield */}
      <mesh position={[0, 0.37, 0.5]}>
        <boxGeometry args={[0.8, 0.001, 0.3]} />
        <meshStandardMaterial color="#ffffff" />
      </mesh>
      
      {/* Antenna PCB trace pattern */}
      <mesh position={[0, 0.08, -1.8]}>
        <boxGeometry args={[1.2, 0.03, 0.8]} />
        <meshStandardMaterial color="#1a1a2e" />
      </mesh>
      
      {/* Zigzag antenna pattern */}
      {[0, 1, 2, 3].map((i) => (
        <mesh key={`ant-${i}`} position={[(i % 2 === 0 ? -0.3 : 0.3), 0.1, -1.8 + i * 0.15]}>
          <boxGeometry args={[0.5, 0.02, 0.08]} />
          <meshStandardMaterial color="#d4af37" metalness={0.8} roughness={0.2} />
        </mesh>
      ))}
      
      {/* Micro USB Port */}
      <mesh position={[0, 0.15, -2.4]}>
        <boxGeometry args={[0.4, 0.15, 0.3]} />
        <meshStandardMaterial color="#2a2a2a" />
      </mesh>
      
      {/* USB metal shell */}
      <mesh position={[0, 0.15, -2.55]}>
        <boxGeometry args={[0.38, 0.12, 0.1]} />
        <meshStandardMaterial color="#c0c0c0" metalness={0.9} roughness={0.1} />
      </mesh>
      
      {/* Reset Button (left side) */}
      <mesh position={[-0.9, 0.15, -1.2]}>
        <cylinderGeometry args={[0.12, 0.12, 0.12, 16]} />
        <meshStandardMaterial color="#1a1a1a" />
      </mesh>
      <mesh position={[-0.9, 0.21, -1.2]}>
        <cylinderGeometry args={[0.1, 0.1, 0.02, 16]} />
        <meshStandardMaterial color="#2a2a2a" />
      </mesh>
      
      {/* Boot Button (right side) */}
      <mesh position={[0.9, 0.15, -1.2]}>
        <cylinderGeometry args={[0.12, 0.12, 0.12, 16]} />
        <meshStandardMaterial color="#1a1a1a" />
      </mesh>
      <mesh position={[0.9, 0.21, -1.2]}>
        <cylinderGeometry args={[0.1, 0.1, 0.02, 16]} />
        <meshStandardMaterial color="#2a2a2a" />
      </mesh>
      
      {/* Power LED (Red) */}
      <mesh position={[-0.6, 0.12, -1.5]}>
        <boxGeometry args={[0.08, 0.08, 0.12]} />
        <meshStandardMaterial 
          color="#ff0000" 
          emissive="#ff0000" 
          emissiveIntensity={0.6}
        />
      </mesh>
      
      {/* User LED (Blue) */}
      <mesh position={[0.6, 0.12, -1.5]}>
        <boxGeometry args={[0.08, 0.08, 0.12]} />
        <meshStandardMaterial 
          color="#0066ff" 
          emissive="#0066ff" 
          emissiveIntensity={0.5}
        />
      </mesh>
      
      {/* Voltage Regulator */}
      <mesh position={[-0.7, 0.1, 0]}>
        <boxGeometry args={[0.2, 0.08, 0.25]} />
        <meshStandardMaterial color="#1a1a1a" metalness={0.3} />
      </mesh>
      
      {/* CP2102 USB-to-Serial chip */}
      <mesh position={[0.5, 0.08, -1.2]}>
        <boxGeometry args={[0.25, 0.05, 0.25]} />
        <meshStandardMaterial color="#1a1a1a" />
      </mesh>
      
      {/* Capacitors */}
      {[-0.4, 0, 0.4].map((z, i) => (
        <mesh key={`cap-${i}`} position={[0.8, 0.12, z]}>
          <cylinderGeometry args={[0.08, 0.08, 0.15, 16]} />
          <meshStandardMaterial color="#4a4a4a" metalness={0.5} />
        </mesh>
      ))}
      
      {/* Left Pin Row (15 pins) */}
      {leftPins.map((pin, i) => {
        const spacing = 0.254; // Standard 2.54mm pin spacing
        const startZ = 1.9;
        return (
          <group key={`left-${pin.pin}`}>
            {/* Pin body above board */}
            <mesh position={[-1.2, 0.3, startZ - i * spacing]} castShadow>
              <cylinderGeometry args={[0.032, 0.032, 0.5, 12]} />
              <meshStandardMaterial 
                color={getThreeColor(pin)} 
                metalness={0.8} 
                roughness={0.2}
              />
            </mesh>
            {/* Pin below board */}
            <mesh position={[-1.2, -0.25, startZ - i * spacing]}>
              <cylinderGeometry args={[0.032, 0.032, 0.4, 12]} />
              <meshStandardMaterial color="#3a3a3a" metalness={0.7} />
            </mesh>
            {/* Solder pad on PCB */}
            <mesh position={[-1.2, 0.055, startZ - i * spacing]}>
              <cylinderGeometry args={[0.06, 0.06, 0.01, 16]} />
              <meshStandardMaterial color="#d4af37" metalness={0.8} />
            </mesh>
          </group>
        );
      })}
      
      {/* Right Pin Row (15 pins) */}
      {rightPins.map((pin, i) => {
        const spacing = 0.254;
        const startZ = 1.9;
        return (
          <group key={`right-${pin.pin}`}>
            {/* Pin body above board */}
            <mesh position={[1.2, 0.3, startZ - i * spacing]} castShadow>
              <cylinderGeometry args={[0.032, 0.032, 0.5, 12]} />
              <meshStandardMaterial 
                color={getThreeColor(pin)} 
                metalness={0.8} 
                roughness={0.2}
              />
            </mesh>
            {/* Pin below board */}
            <mesh position={[1.2, -0.25, startZ - i * spacing]}>
              <cylinderGeometry args={[0.032, 0.032, 0.4, 12]} />
              <meshStandardMaterial color="#3a3a3a" metalness={0.7} />
            </mesh>
            {/* Solder pad on PCB */}
            <mesh position={[1.2, 0.055, startZ - i * spacing]}>
              <cylinderGeometry args={[0.06, 0.06, 0.01, 16]} />
              <meshStandardMaterial color="#d4af37" metalness={0.8} />
            </mesh>
          </group>
        );
      })}
      
      {/* Pin labels simulation (white text areas) */}
      <mesh position={[-1.0, 0.056, 0]}>
        <boxGeometry args={[0.3, 0.001, 4.5]} />
        <meshStandardMaterial color="#ffffff" transparent opacity={0.8} />
      </mesh>
      <mesh position={[1.0, 0.056, 0]}>
        <boxGeometry args={[0.3, 0.001, 4.5]} />
        <meshStandardMaterial color="#ffffff" transparent opacity={0.8} />
      </mesh>
    </group>
  );
}

/* =========================
   DIGITAL TWIN COMPONENT
   ========================= */
export default function DigitalTwin() {
  const [pins, setPins] = useState(DEFAULT_PINS);

  /* Simulated backend update */
  useEffect(() => {
    const timeout = setTimeout(() => {
      setPins(pins =>
        pins.map(pin => ({
          ...pin,
          used: Math.random() > 0.4,
          working: Math.random() > 0.3
        }))
      );
    }, 3000);
    
    return () => clearTimeout(timeout);
  }, []);

  return (
    <div className="w-full h-screen bg-gray-900 p-4">
      <div className="bg-gray-800 rounded-lg border border-green-700 p-4 flex flex-col gap-4 h-full">
        <h3 className="text-lg font-semibold text-green-400">
          ESP32 DevKit Digital Twin - Real-time Monitoring
        </h3>
        
        {/* 3D MODEL (≈65%) */}
        <div className="flex-1 border-2 border-gray-700 rounded-lg bg-gradient-to-b from-gray-800 to-gray-900 overflow-hidden shadow-2xl">
          <Canvas shadows>
            <PerspectiveCamera makeDefault position={[5, 4, 5]} fov={45} />
            
            {/* Lighting setup for realistic rendering */}
            <ambientLight intensity={0.3} />
            <directionalLight 
              position={[5, 8, 5]} 
              intensity={1.2} 
              castShadow
              shadow-mapSize-width={2048}
              shadow-mapSize-height={2048}
            />
            <directionalLight position={[-5, 5, -5]} intensity={0.4} />
            <pointLight position={[0, 3, 0]} intensity={0.6} color="#ffffff" />
            <spotLight position={[0, 5, 0]} intensity={0.5} angle={0.6} penumbra={1} castShadow />
            
            {/* ESP32 Model */}
            <ESP32Model pins={pins} />
            
            {/* Ground plane with grid */}
            <mesh rotation={[-Math.PI / 2, 0, 0]} position={[0, -0.6, 0]} receiveShadow>
              <planeGeometry args={[20, 20]} />
              <meshStandardMaterial color="#2a2a2a" />
            </mesh>
            
            <gridHelper args={[20, 40, '#3a3a3a', '#2a2a2a']} position={[0, -0.59, 0]} />
            
            <OrbitControls 
              enableDamping 
              dampingFactor={0.05}
              minDistance={3}
              maxDistance={12}
              maxPolarAngle={Math.PI / 2}
            />
          </Canvas>
        </div>
        
        {/* PIN STATUS GRID - 5 columns x 8 rows */}
        <div className="bg-gray-900 rounded-lg border border-gray-700 p-3">
          <div className="grid grid-cols-5 gap-2 text-xs">
            {pins.map((pin, idx) => (
              <div
                key={pin.pin}
                className="flex items-center gap-2 bg-gray-800 px-3 py-2 rounded border border-gray-700 hover:border-green-600 transition-all cursor-pointer"
                title={`${pin.pin} - ${!pin.used ? 'Unused' : pin.working ? 'Working' : 'Error'}`}
              >
                <span className={`w-3 h-3 rounded-full ${getPinColor(pin)} shadow-lg animate-pulse`} />
                <span className="text-gray-200 font-mono font-semibold">{pin.pin}</span>
              </div>
            ))}
          </div>
        </div>
        
        {/* Legend and Stats */}
        <div className="flex items-center justify-between text-xs text-gray-400 border-t border-gray-700 pt-3">
          <div className="flex gap-6">
            <div className="flex items-center gap-2">
              <span className="w-3 h-3 rounded-full bg-gray-400 shadow" />
              <span>Unused ({pins.filter(p => !p.used).length})</span>
            </div>
            <div className="flex items-center gap-2">
              <span className="w-3 h-3 rounded-full bg-green-500 shadow" />
              <span>Working ({pins.filter(p => p.used && p.working).length})</span>
            </div>
            <div className="flex items-center gap-2">
              <span className="w-3 h-3 rounded-full bg-red-500 shadow" />
              <span>Error ({pins.filter(p => p.used && !p.working).length})</span>
            </div>
          </div>
          <div className="text-green-400 font-mono">
            Status: Live Monitoring
          </div>
        </div>
      </div>
    </div>
  );
}