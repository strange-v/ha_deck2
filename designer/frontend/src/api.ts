import type { ConfigurationSummary, LayoutDocument, Position, Size } from "./types";

async function responseJson<T>(response: Response): Promise<T> {
  if (response.ok) return response.json() as Promise<T>;
  const payload = await response.json().catch(() => null);
  const message = payload?.detail?.message ?? `${response.status} ${response.statusText}`;
  throw new Error(message);
}

export async function listConfigurations(): Promise<ConfigurationSummary[]> {
  return responseJson(await fetch("/api/configurations"));
}

export async function loadLayout(path: string): Promise<LayoutDocument> {
  return responseJson(await fetch(`/api/layout?path=${encodeURIComponent(path)}`));
}

export async function saveLayout(
  document: LayoutDocument,
  changes: Array<{ screenId: string; widgetId: string; position?: Position; size?: Size }>,
): Promise<{ revision: string; diff: string }> {
  return responseJson(
    await fetch("/api/layout", {
      method: "PATCH",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        path: document.path,
        revision: document.revision,
        changes: changes.map(({ screenId, widgetId, position, size }) => ({
          screen_id: screenId,
          widget_id: widgetId,
          ...(position ? { x: position.x, y: position.y } : {}),
          ...(size ? { width: size.width, height: size.height } : {}),
        })),
      }),
    }),
  );
}

export function configAssetUrl(configuration: string, asset: string): string {
  const params = new URLSearchParams({ configuration, asset });
  return `/api/config-asset?${params}`;
}
