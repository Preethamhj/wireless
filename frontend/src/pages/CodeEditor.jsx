import EditorPanel from "../components/EditorPanel";
import Terminal from "../components/Terminal";

export default function CodeEditor() {
  return (
    <div className="p-6 grid grid-cols-1 md:grid-cols-3 gap-6">
      <EditorPanel />
      <Terminal />
    </div>
  );
}
