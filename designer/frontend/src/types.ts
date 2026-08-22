export interface FontResource {
  id: string;
  size: number;
  file?: string | null;
}

export interface ThemeFonts {
  id: string;
  base: string;
  body_font?: FontResource | null;
  fonts: Record<string, FontResource>;
  colors: Record<string, string>;
  accents?: Record<string, { color: string; on_color: string }>;
}

export interface Geometry {
  x: number | null;
  y: number | null;
  width: number | null;
  height: number | null;
}

export interface WidgetPreviewData {
  text?: string | null;
  label?: string | null;
  glyph?: string | null;
  variant?: string | null;
  accent?: string | null;
  background_color?: string | null;
  text_color?: string | null;
  border_color?: string | null;
  border_width?: number | null;
  radius?: number | null;
  orientation?: string | null;
  align?: string | null;
  margin?: number | null;
  units?: string | null;
  format?: string | null;
  action_height?: number | null;
  power_glyph?: string | null;
  explicit_fonts: Record<string, FontResource>;
}

export interface LayoutWidget {
  key: string;
  type: string;
  id?: string | null;
  geometry: Geometry;
  movable: boolean;
  resizable: boolean;
  read_only_reason?: string | null;
  preview: WidgetPreviewData;
}

export interface LayoutScreen {
  id: string;
  background_color?: string | null;
  background_image?: string | null;
  background_image_file?: string | null;
  widgets: LayoutWidget[];
}

export interface LayoutDocument {
  path: string;
  revision: string;
  deck_id: string;
  default_screen: string;
  default_theme: string;
  viewport: { width: number; height: number; display_id: string };
  themes: ThemeFonts[];
  screens: LayoutScreen[];
  warnings: string[];
}

export interface ConfigurationSummary {
  path: string;
  has_ha_deck: boolean;
  error?: string | null;
}

export interface Position {
  x: number;
  y: number;
}

export interface Size {
  width: number;
  height: number;
}
