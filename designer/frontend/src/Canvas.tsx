import { useMemo, useRef, useState } from "react";

import { configAssetUrl } from "./api";
import { useEditorStore } from "./store";
import type { LayoutScreen, LayoutWidget, Position, ThemeFonts } from "./types";
import WidgetPreview from "./WidgetPreview";

interface Point { x: number; y: number }
interface Rect extends Point { width: number; height: number }

function effectiveRect(widget: LayoutWidget, position: Position | undefined, size: { width: number; height: number } | undefined, viewport: { width: number; height: number }): Rect {
  const width = size?.width ?? widget.geometry.width ?? 56;
  const height = size?.height ?? widget.geometry.height ?? 56;
  if (position) return { ...position, width, height };
  const margin = widget.preview.margin ?? 8;
  switch (widget.preview.align) {
    case "top_right": return { x: viewport.width - width - margin, y: margin, width, height };
    case "bottom_left": return { x: margin, y: viewport.height - height - margin, width, height };
    case "bottom_right": return { x: viewport.width - width - margin, y: viewport.height - height - margin, width, height };
    case "center": return { x: (viewport.width - width) / 2, y: (viewport.height - height) / 2, width, height };
    default: return { x: margin, y: margin, width, height };
  }
}

const intersects = (a: Rect, b: Rect) =>
  a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y;

export default function Canvas({ screen, theme, zoom }: { screen: LayoutScreen; theme?: ThemeFonts; zoom: number }) {
  const document = useEditorStore((state) => state.document)!;
  const positions = useEditorStore((state) => state.positions);
  const sizes = useEditorStore((state) => state.sizes);
  const selected = useEditorStore((state) => state.selected);
  const select = useEditorStore((state) => state.select);
  const selectMany = useEditorStore((state) => state.selectMany);
  const beginOperation = useEditorStore((state) => state.beginOperation);
  const moveOperation = useEditorStore((state) => state.moveOperation);
  const commitOperation = useEditorStore((state) => state.commitOperation);
  const canvasRef = useRef<HTMLDivElement>(null);
  const [marquee, setMarquee] = useState<Rect | null>(null);
  const marqueeOrigin = useRef<Point | null>(null);

  const widgetRects = useMemo(() => Object.fromEntries(screen.widgets.map((widget) => [
    widget.key, effectiveRect(widget, positions[widget.key], sizes[widget.key], document.viewport),
  ])), [document.viewport, positions, screen.widgets, sizes]);

  const canvasPoint = (event: React.PointerEvent): Point => {
    const bounds = canvasRef.current!.getBoundingClientRect();
    return { x: (event.clientX - bounds.left) / zoom, y: (event.clientY - bounds.top) / zoom };
  };

  const startWidgetDrag = (event: React.PointerEvent, widget: LayoutWidget) => {
    event.stopPropagation();
    select(widget.key, event.ctrlKey || event.metaKey);
    if (!widget.movable) return;
    const start = { x: event.clientX, y: event.clientY };
    beginOperation();
    const move = (moveEvent: PointerEvent) => moveOperation(
      Math.round((moveEvent.clientX - start.x) / zoom),
      Math.round((moveEvent.clientY - start.y) / zoom),
    );
    const up = () => {
      commitOperation();
      window.removeEventListener("pointermove", move);
      window.removeEventListener("pointerup", up);
    };
    window.addEventListener("pointermove", move);
    window.addEventListener("pointerup", up, { once: true });
  };

  return <div className="canvas-scroll">
    <div className="canvas-scale" style={{ width: document.viewport.width * zoom, height: document.viewport.height * zoom }}>
      <div
        ref={canvasRef}
        className="deck-canvas"
        style={{
          width: document.viewport.width,
          height: document.viewport.height,
          transform: `scale(${zoom})`,
          backgroundColor: screen.background_color ?? theme?.colors.background_color ?? "#101418",
          backgroundImage: screen.background_image_file
            ? `url("${configAssetUrl(document.path, screen.background_image_file)}")`
            : undefined,
          backgroundPosition: "center",
          backgroundRepeat: "no-repeat",
        }}
        onPointerDown={(event) => {
          if (event.target !== event.currentTarget) return;
          const origin = canvasPoint(event);
          marqueeOrigin.current = origin;
          setMarquee({ ...origin, width: 0, height: 0 });
          event.currentTarget.setPointerCapture(event.pointerId);
        }}
        onPointerMove={(event) => {
          if (!marqueeOrigin.current) return;
          const point = canvasPoint(event);
          const origin = marqueeOrigin.current;
          setMarquee({ x: Math.min(origin.x, point.x), y: Math.min(origin.y, point.y), width: Math.abs(point.x - origin.x), height: Math.abs(point.y - origin.y) });
        }}
        onPointerUp={(event) => {
          if (!marqueeOrigin.current || !marquee) return;
          const keys = marquee.width < 2 && marquee.height < 2 ? [] : screen.widgets.filter((widget) => intersects(widgetRects[widget.key], marquee)).map((widget) => widget.key);
          selectMany(keys);
          marqueeOrigin.current = null;
          setMarquee(null);
          event.currentTarget.releasePointerCapture(event.pointerId);
        }}
      >
        {screen.widgets.map((widget) => {
          const rect = widgetRects[widget.key];
          return <div
            key={widget.key}
            className={`widget-shell ${selected.includes(widget.key) ? "selected" : ""} ${widget.movable ? "movable" : "locked"}`}
            style={{ left: rect.x, top: rect.y, width: rect.width, height: rect.height }}
            title={widget.read_only_reason ?? widget.id ?? widget.type}
            onPointerDown={(event) => startWidgetDrag(event, widget)}
          >
            <WidgetPreview widget={widget} theme={theme} />
          </div>;
        })}
        {marquee && <div className="marquee" style={marquee} />}
      </div>
    </div>
  </div>;
}
