import { beforeEach, describe, expect, it } from "vitest";

import { changedPositions, useEditorStore } from "./store";
import type { LayoutDocument } from "./types";

const document: LayoutDocument = {
  path: "dashboard.yaml",
  revision: "sha256:original",
  deck_id: "deck",
  default_screen: "main",
  default_theme: "dark",
  viewport: { width: 320, height: 240, display_id: "display" },
  themes: [{ id: "dark", base: "dark", fonts: {}, colors: {} }],
  warnings: [],
  screens: [{
    id: "main",
    widgets: [
      { key: "main/a", type: "button", id: "a", movable: true, resizable: true, geometry: { x: 0, y: 0, width: 10, height: 10 }, preview: { explicit_fonts: {} } },
      { key: "main/b", type: "button", id: "b", movable: true, resizable: true, geometry: { x: 20, y: 20, width: 10, height: 10 }, preview: { explicit_fonts: {} } },
    ],
  }],
};

describe("editor store", () => {
  beforeEach(() => useEditorStore.getState().setDocument(document));

  it("moves a multi-selection by the same pixel delta", () => {
    const store = useEditorStore.getState();
    store.selectMany(["main/a", "main/b"]);
    useEditorStore.getState().nudge(8, -1);

    expect(useEditorStore.getState().positions).toMatchObject({
      "main/a": { x: 8, y: -1 },
      "main/b": { x: 28, y: 19 },
    });
    expect(changedPositions(useEditorStore.getState())).toHaveLength(2);
  });

  it("undoes and redoes movement", () => {
    useEditorStore.getState().select("main/a", false);
    useEditorStore.getState().nudge(1, 0);
    useEditorStore.getState().undo();
    expect(useEditorStore.getState().positions["main/a"].x).toBe(0);
    useEditorStore.getState().redo();
    expect(useEditorStore.getState().positions["main/a"].x).toBe(1);
  });
});
