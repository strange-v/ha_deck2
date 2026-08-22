import { create } from "zustand";

import type { LayoutDocument, Position, Size } from "./types";

interface EditorSnapshot {
  positions: Record<string, Position>;
  sizes: Record<string, Size>;
}

interface EditorState {
  document: LayoutDocument | null;
  screenId: string | null;
  positions: Record<string, Position>;
  originalPositions: Record<string, Position>;
  sizes: Record<string, Size>;
  originalSizes: Record<string, Size>;
  selected: string[];
  past: EditorSnapshot[];
  future: EditorSnapshot[];
  operationStart: EditorSnapshot | null;
  setDocument: (document: LayoutDocument) => void;
  setScreen: (screenId: string) => void;
  select: (key: string, additive: boolean) => void;
  selectMany: (keys: string[]) => void;
  clearSelection: () => void;
  beginOperation: () => void;
  moveOperation: (dx: number, dy: number) => void;
  commitOperation: () => void;
  nudge: (dx: number, dy: number) => void;
  setSinglePosition: (key: string, position: Position) => void;
  setSingleSize: (key: string, size: Size) => void;
  undo: () => void;
  redo: () => void;
  acceptSavedRevision: (revision: string) => void;
}

const clonePositions = (value: Record<string, Position>): Record<string, Position> =>
  Object.fromEntries(Object.entries(value).map(([key, position]) => [key, { ...position }]));
const cloneSizes = (value: Record<string, Size>): Record<string, Size> =>
  Object.fromEntries(Object.entries(value).map(([key, size]) => [key, { ...size }]));
const snapshot = (state: Pick<EditorState, "positions" | "sizes">): EditorSnapshot => ({
  positions: clonePositions(state.positions), sizes: cloneSizes(state.sizes),
});

const documentPositions = (document: LayoutDocument): Record<string, Position> =>
  Object.fromEntries(
    document.screens.flatMap((screen) =>
      screen.widgets
        .filter((widget) => widget.geometry.x !== null && widget.geometry.y !== null)
        .map((widget) => [
          widget.key,
          { x: widget.geometry.x as number, y: widget.geometry.y as number },
        ]),
    ),
  );
const documentSizes = (document: LayoutDocument): Record<string, Size> =>
  Object.fromEntries(document.screens.flatMap((screen) => screen.widgets
    .filter((widget) => widget.geometry.width !== null && widget.geometry.height !== null)
    .map((widget) => [widget.key, { width: widget.geometry.width as number, height: widget.geometry.height as number }])));

export const useEditorStore = create<EditorState>((set, get) => ({
  document: null,
  screenId: null,
  positions: {},
  originalPositions: {},
  sizes: {},
  originalSizes: {},
  selected: [],
  past: [],
  future: [],
  operationStart: null,
  setDocument: (document) => {
    const positions = documentPositions(document);
    const sizes = documentSizes(document);
    set({
      document,
      screenId: document.default_screen,
      positions,
      originalPositions: clonePositions(positions),
      sizes,
      originalSizes: cloneSizes(sizes),
      selected: [],
      past: [],
      future: [],
      operationStart: null,
    });
  },
  setScreen: (screenId) => set({ screenId, selected: [] }),
  select: (key, additive) =>
    set((state) => ({
      selected: additive
        ? state.selected.includes(key)
          ? state.selected.filter((item) => item !== key)
          : [...state.selected, key]
        : state.selected.includes(key) && state.selected.length === 1
          ? state.selected
          : [key],
    })),
  selectMany: (keys) => set({ selected: keys }),
  clearSelection: () => set({ selected: [] }),
  beginOperation: () => set((state) => ({ operationStart: snapshot(state) })),
  moveOperation: (dx, dy) => {
    const state = get();
    if (!state.operationStart) return;
    const positions = clonePositions(state.operationStart.positions);
    for (const key of state.selected) {
      const initial = state.operationStart.positions[key];
      if (initial) positions[key] = { x: initial.x + dx, y: initial.y + dy };
    }
    set({ positions });
  },
  commitOperation: () => {
    const state = get();
    if (!state.operationStart) return;
    const changed = state.selected.some((key) => {
      const before = state.operationStart?.positions[key];
      const after = state.positions[key];
      return before && after && (before.x !== after.x || before.y !== after.y);
    });
    set({
      operationStart: null,
      past: changed ? [...state.past, state.operationStart] : state.past,
      future: changed ? [] : state.future,
    });
  },
  nudge: (dx, dy) => {
    const state = get();
    if (!state.selected.length) return;
    const before = snapshot(state);
    const positions = clonePositions(state.positions);
    for (const key of state.selected) {
      const position = positions[key];
      if (position) positions[key] = { x: position.x + dx, y: position.y + dy };
    }
    set({ positions, past: [...state.past, before], future: [] });
  },
  setSinglePosition: (key, position) => {
    const state = get();
    const before = snapshot(state);
    set({
      positions: { ...state.positions, [key]: position },
      past: [...state.past, before],
      future: [],
    });
  },
  setSingleSize: (key, size) => {
    const state = get();
    const before = snapshot(state);
    set({ sizes: { ...state.sizes, [key]: size }, past: [...state.past, before], future: [] });
  },
  undo: () => {
    const state = get();
    const previous = state.past.at(-1);
    if (!previous) return;
    set({
      positions: clonePositions(previous.positions),
      sizes: cloneSizes(previous.sizes),
      past: state.past.slice(0, -1),
      future: [snapshot(state), ...state.future],
    });
  },
  redo: () => {
    const state = get();
    const next = state.future[0];
    if (!next) return;
    set({
      positions: clonePositions(next.positions),
      sizes: cloneSizes(next.sizes),
      past: [...state.past, snapshot(state)],
      future: state.future.slice(1),
    });
  },
  acceptSavedRevision: (revision) =>
    set((state) => ({
      document: state.document ? { ...state.document, revision } : null,
      originalPositions: clonePositions(state.positions),
      originalSizes: cloneSizes(state.sizes),
      past: [],
      future: [],
    })),
}));

export function changedPositions(state: EditorState) {
  if (!state.document) return [];
  return state.document.screens.flatMap((screen) =>
    screen.widgets.flatMap((widget) => {
      if (!widget.id) return [];
      const current = state.positions[widget.key];
      const original = state.originalPositions[widget.key];
      const currentSize = state.sizes[widget.key];
      const originalSize = state.originalSizes[widget.key];
      const positionChanged = widget.movable && current && original && (current.x !== original.x || current.y !== original.y);
      const sizeChanged = widget.resizable && currentSize && originalSize && (currentSize.width !== originalSize.width || currentSize.height !== originalSize.height);
      if (!positionChanged && !sizeChanged) return [];
      return [{ screenId: screen.id, widgetId: widget.id, position: positionChanged ? current : undefined, size: sizeChanged ? currentSize : undefined }];
    }),
  );
}
