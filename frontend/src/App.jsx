import { useState } from "react";
import Navbar from "./components/Navbar";
import Dashboard from "./pages/Dashboard";
import CodeEditor from "./pages/CodeEditor";

export default function App() {
  const [active, setActive] = useState("Dashboard");

  return (
    <>
      <Navbar active={active} setActive={setActive} />
      {active === "Dashboard" ? <Dashboard /> : <CodeEditor />}
    </>
  );
}
