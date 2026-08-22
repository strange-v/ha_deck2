import { useEffect, useMemo, useState } from "react";

import { configAssetUrl, listConfigurations, loadLayout, saveLayout } from "./api";
import Canvas from "./Canvas";
import NumericExpressionInput from "./NumericExpressionInput";
import { changedPositions, useEditorStore } from "./store";
import type { ConfigurationSummary, FontResource } from "./types";

const loadedFontFamilies = new Set<string>();

async function loadDeclaredFonts(configuration: string, fonts: FontResource[]) {
  const unique = new Map(fonts.filter((font) => font.file).map((font) => [font.id, font]));
  await Promise.all([...unique.values()].map(async (font) => {
    const family = `ha-deck-${font.id}`;
    if (loadedFontFamilies.has(family)) return;
    const face = new FontFace(family, `url(${configAssetUrl(configuration, font.file!)})`);
    await face.load();
    document.fonts.add(face);
    loadedFontFamilies.add(family);
  }));
}

export default function App() {
  const document = useEditorStore((state) => state.document);
  const screenId = useEditorStore((state) => state.screenId);
  const selected = useEditorStore((state) => state.selected);
  const positions = useEditorStore((state) => state.positions);
  const sizes = useEditorStore((state) => state.sizes);
  const setDocument = useEditorStore((state) => state.setDocument);
  const setScreen = useEditorStore((state) => state.setScreen);
  const nudge = useEditorStore((state) => state.nudge);
  const setSinglePosition = useEditorStore((state) => state.setSinglePosition);
  const setSingleSize = useEditorStore((state) => state.setSingleSize);
  const undo = useEditorStore((state) => state.undo);
  const redo = useEditorStore((state) => state.redo);
  const acceptSavedRevision = useEditorStore((state) => state.acceptSavedRevision);
  const [configurations, setConfigurations] = useState<ConfigurationSummary[]>([]);
  const [selectedPath, setSelectedPath] = useState("");
  const [zoom, setZoom] = useState(1);
  const [error, setError] = useState<string | null>(null);
  const [busy, setBusy] = useState(false);
  const [lastDiff, setLastDiff] = useState("");

  const screen = document?.screens.find((item) => item.id === screenId);
  const theme = document?.themes.find((item) => item.id === document.default_theme);
  const changes = useMemo(
    () => changedPositions(useEditorStore.getState()),
    [document, positions, sizes],
  );
  const selectedWidgets = useMemo(
    () => screen?.widgets.filter((widget) => selected.includes(widget.key)) ?? [],
    [screen, selected],
  );

  const open = async (path: string) => {
    if (!path) return;
    setBusy(true);
    setError(null);
    setLastDiff("");
    try {
      const layout = await loadLayout(path);
      setDocument(layout);
      const fonts = layout.themes.flatMap((item) => [
        ...(item.body_font ? [item.body_font] : []),
        ...Object.values(item.fonts),
      ]);
      loadDeclaredFonts(path, fonts).catch((fontError) =>
        setError(`Layout loaded, but a preview font failed: ${String(fontError)}`),
      );
    } catch (loadError) {
      setError(loadError instanceof Error ? loadError.message : String(loadError));
    } finally {
      setBusy(false);
    }
  };

  useEffect(() => {
    listConfigurations().then((items) => {
      setConfigurations(items);
      const first = items.find((item) => item.has_ha_deck && !item.error);
      if (first) {
        setSelectedPath(first.path);
        void open(first.path);
      }
    }).catch((loadError) => setError(String(loadError)));
  }, []);

  useEffect(() => {
    const keydown = (event: KeyboardEvent) => {
      if (event.target instanceof HTMLInputElement || event.target instanceof HTMLSelectElement) return;
      if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "z") {
        event.preventDefault();
        event.shiftKey ? redo() : undo();
        return;
      }
      const step = event.shiftKey ? 8 : 1;
      const deltas: Record<string, [number, number]> = {
        ArrowLeft: [-step, 0], ArrowRight: [step, 0], ArrowUp: [0, -step], ArrowDown: [0, step],
      };
      const delta = deltas[event.key];
      if (delta) {
        event.preventDefault();
        nudge(...delta);
      }
    };
    window.addEventListener("keydown", keydown);
    return () => window.removeEventListener("keydown", keydown);
  }, [nudge, redo, undo]);

  const save = async () => {
    if (!document || !changes.length) return;
    setBusy(true);
    setError(null);
    try {
      const result = await saveLayout(document, changes);
      acceptSavedRevision(result.revision);
      setLastDiff(result.diff);
    } catch (saveError) {
      setError(saveError instanceof Error ? saveError.message : String(saveError));
    } finally {
      setBusy(false);
    }
  };

  return <div className="app-shell">
    <header className="topbar">
      <div className="brand"><strong>HA Deck</strong><span>Layout Editor</span></div>
      <label className="configuration-picker"><span>Configuration</span>
        <select value={selectedPath} onChange={(event) => { setSelectedPath(event.target.value); void open(event.target.value); }}>
          {configurations.filter((item) => item.has_ha_deck).map((item) => <option key={item.path} title={item.error ?? undefined}>{item.error ? "⚠ " : ""}{item.path}</option>)}
        </select>
      </label>
      <div className="top-actions">
        <button disabled={!document || busy} onClick={() => document && void open(document.path)}>Reload</button>
        <button className="primary" disabled={!changes.length || busy} onClick={() => void save()}>Save{changes.length ? ` (${changes.length})` : ""}</button>
      </div>
    </header>

    <aside className="sidebar screens-panel">
      <h2>Screens</h2>
      {document?.screens.map((item) => <button key={item.id} className={item.id === screenId ? "active" : ""} onClick={() => setScreen(item.id)}>
        <span>{item.id}</span><small>{item.widgets.length}</small>
      </button>)}
      {document && <div className="document-meta"><span>{document.viewport.width} × {document.viewport.height}</span><small>{document.viewport.display_id}</small></div>}
    </aside>

    <main className="workspace">
      <div className="workspace-toolbar"><span>{screen?.id ?? "No screen"}</span><label>Zoom
        <select value={zoom} onChange={(event) => setZoom(Number(event.target.value))}>
          <option value={0.5}>50%</option><option value={0.75}>75%</option><option value={1}>100%</option><option value={1.25}>125%</option><option value={1.5}>150%</option>
        </select>
      </label></div>
      {error && <div className="error-banner">{error}</div>}
      {document?.warnings.map((warning) => <div className="warning-banner" key={warning}>{warning}</div>)}
      {screen && <Canvas screen={screen} theme={theme} zoom={zoom} />}
    </main>

    <aside className="sidebar inspector">
      <h2>Selection</h2>
      {!selectedWidgets.length && <p className="muted">Select one or more widgets.</p>}
      {selectedWidgets.length === 1 && (() => {
        const widget = selectedWidgets[0];
        const position = positions[widget.key];
        const size = sizes[widget.key];
        return <div className="property-group"><strong>{widget.id ?? widget.type}</strong><small>{widget.type}</small>
          {position ? <>
            <label>X<NumericExpressionInput value={position.x} disabled={!widget.movable} onCommit={(x) => setSinglePosition(widget.key, { ...position, x })} /></label>
            <label>Y<NumericExpressionInput value={position.y} disabled={!widget.movable} onCommit={(y) => setSinglePosition(widget.key, { ...position, y })} /></label>
          </> : <p className="muted">{widget.read_only_reason}</p>}
          <label>Width<NumericExpressionInput value={size?.width ?? "auto"} min={1} disabled={!widget.resizable} onCommit={(width) => size && setSingleSize(widget.key, { ...size, width })} /></label>
          <label>Height<NumericExpressionInput value={size?.height ?? "auto"} min={1} disabled={!widget.resizable} onCommit={(height) => size && setSingleSize(widget.key, { ...size, height })} /></label>
        </div>;
      })()}
      {selectedWidgets.length > 1 && <div className="property-group"><strong>{selectedWidgets.length} widgets</strong><p className="muted">Drag or use arrow keys. Shift moves by 8 px.</p></div>}
      {lastDiff && <details><summary>Last saved diff</summary><pre>{lastDiff}</pre></details>}
    </aside>
  </div>;
}
