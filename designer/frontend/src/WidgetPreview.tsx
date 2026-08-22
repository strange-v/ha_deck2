import type { CSSProperties, ReactNode } from "react";

import type { FontResource, LayoutWidget, ThemeFonts } from "./types";

const fallback = { surface: "#111318", onSurface: "#e2e2e9", primary: "#a8c7fa", onPrimary: "#062e6f" };
const family = (font?: FontResource | null) => font ? `"ha-deck-${font.id}"` : undefined;
const token = (theme: ThemeFonts | undefined, key: string) =>
  theme?.fonts[key] ?? theme?.body_font ?? undefined;
const dummyValue = (units?: string | null) => {
  const normalized = units?.trim().toLowerCase() ?? "";
  if (normalized.includes("°")) return "21.5";
  if (normalized === "%") return "45";
  if (normalized === "ppm") return "750";
  return "21.5";
};

function Icon({ glyph, font, slot = false }: { glyph?: string | null; font?: FontResource; slot?: boolean }) {
  return glyph ? <span className={`widget-icon${slot ? " icon-slot" : ""}`} style={{
    fontFamily: family(font),
    fontSize: font?.size,
    ...(slot && font?.size ? { width: font.size, height: font.size } : {}),
  }}>{glyph}</span> : null;
}

function Frame({ widget, theme, children, className = "", style }: {
  widget: LayoutWidget; theme?: ThemeFonts; children: ReactNode; className?: string; style?: CSSProperties;
}) {
  const preview = widget.preview;
  const frameStyle: CSSProperties = {
    color: preview.text_color ?? theme?.colors.on_surface_color ?? fallback.onSurface,
    background: preview.background_color ?? theme?.colors.surface_color ?? fallback.surface,
    borderColor: preview.border_color ?? theme?.colors.outline_color ?? "#65717d",
    borderWidth: preview.border_width ?? 0,
    borderRadius: preview.radius ?? 5,
    ...style,
  };
  return <div className={`widget-frame ${className}`} style={frameStyle}>{children}</div>;
}

export default function WidgetPreview({ widget, theme }: { widget: LayoutWidget; theme?: ThemeFonts }) {
  const preview = widget.preview;
  const textFont = preview.explicit_fonts.font ?? token(theme, "text_small");
  const iconFont = preview.explicit_fonts.icon_font ?? token(theme, "icon_medium");
  const valueFont = preview.explicit_fonts.value_font ?? token(theme, "text_medium");
  const textStyle: CSSProperties = { fontFamily: family(textFont), fontSize: textFont?.size };
  const value = dummyValue(preview.units);
  const colors = theme?.colors ?? {};
  const accent = preview.accent === "neutral"
    ? { color: colors.on_surface_color ?? fallback.onSurface, on_color: colors.background_color ?? fallback.surface }
    : theme?.accents?.[preview.accent ?? ""] ?? { color: colors.primary_color ?? fallback.primary, on_color: colors.on_primary_color ?? fallback.onPrimary };

  switch (widget.type) {
    case "button":
      { const variant = preview.variant ?? "glass";
      const buttonStyle: CSSProperties = variant === "filled"
        ? { background: preview.background_color ?? accent.color, color: preview.text_color ?? accent.on_color }
        : variant === "icon"
          ? { background: "transparent", color: preview.text_color ?? colors.on_surface_color }
          : { background: preview.background_color ?? `${colors.on_surface_color ?? fallback.onSurface}40`, color: preview.text_color ?? colors.on_surface_color };
      return <Frame widget={widget} theme={theme} className={`button-preview ${variant}`} style={buttonStyle}>
        <Icon glyph={preview.glyph} font={iconFont} />
        {preview.text && <span style={textStyle}>{preview.text}</span>}
      </Frame>; }
    case "sensor_value":
      return <Frame widget={widget} theme={theme} className="value-preview">
        <Icon glyph={preview.glyph} font={iconFont} slot />
        <div className="value-copy">
          <div className="value-row"><span style={{ fontFamily: family(valueFont), fontSize: valueFont?.size }}>{value}</span>
            {preview.units && <span style={textStyle}>{preview.units}</span>}</div>
          {preview.label && <small style={textStyle}>{preview.label}</small>}
        </div>
      </Frame>;
    case "weather":
      return <Frame widget={widget} theme={theme} className="weather-preview">
        <span className="weather-symbol icon-slot" style={{ fontSize: iconFont?.size ?? 48, width: iconFont?.size ?? 48, height: iconFont?.size ?? 48 }}>☁</span>
        <div className="value-row">
          <span style={{ fontFamily: family(valueFont), fontSize: valueFont?.size }}>{value}</span>
          <span style={textStyle}>{preview.units ?? "°C"}</span>
        </div>
      </Frame>;
    case "slider":
      return <Frame widget={widget} theme={theme} className={`slider-preview ${preview.orientation ?? "vertical"}`} style={{ background: "transparent" }}>
        <div className="slider-track" style={{ background: `${colors.on_surface_color ?? fallback.onSurface}40` }}><div className="slider-fill" style={{ background: accent.color }} /></div>
        {(preview.glyph || preview.label) && <div className="slider-action" style={{ flexBasis: preview.action_height ?? 56 }}><Icon glyph={preview.glyph} font={preview.explicit_fonts.icon_font ?? token(theme, "icon_small")} />{!preview.glyph && <span style={textStyle}>{preview.label}</span>}</div>}
      </Frame>;
    case "climate":
      return <Frame widget={widget} theme={theme} className="climate-preview" style={{ background: "transparent" }}>
        <div className="climate-top">
          <div className="climate-arc" style={{
            background: `conic-gradient(from 225deg, ${accent.color} 0deg 170deg, ${colors.outline_color ?? "#8c9199"}64 170deg 270deg, transparent 270deg 360deg)`,
          }} />
          <div className="climate-readout">
            <span className="climate-current" style={textStyle}>{value}{preview.units ?? "°C"}</span>
            <div className="value-row">
              <span style={{ fontFamily: family(token(theme, "text_large")), fontSize: token(theme, "text_large")?.size }}>{value}</span>
              <span style={textStyle}>{preview.units ?? "°C"}</span>
            </div>
          </div>
          <div className="climate-adjust">
            <span className="climate-adjust-button" style={{ background: `${colors.on_surface_color ?? fallback.onSurface}40` }}>−</span>
            <span className="climate-adjust-button" style={{ background: `${colors.on_surface_color ?? fallback.onSurface}40` }}>+</span>
          </div>
        </div>
        <div className="climate-bottom">
          <div className="climate-modes">
            {[0, 1, 2].map((index) => <span className="climate-placeholder" style={{ background: `${colors.on_surface_color ?? fallback.onSurface}40` }} key={index} />)}
          </div>
          <span className="climate-power" style={{ background: `${colors.on_surface_color ?? fallback.onSurface}40` }}>
            <Icon glyph={preview.power_glyph} font={iconFont} />
          </span>
        </div>
      </Frame>;
    case "navigation_button":
      return <Frame widget={widget} theme={theme} className="navigation-preview glass" style={{
        background: `${colors.on_surface_color ?? fallback.onSurface}40`,
        color: colors.on_surface_color ?? fallback.onSurface,
      }}><Icon glyph={preview.glyph ?? "󰁍"} font={token(theme, "icon_small")} /></Frame>;
    default:
      return <Frame widget={widget} theme={theme} className="generic-preview"><strong>{widget.type}</strong><small>{widget.id ?? "No id"}</small></Frame>;
  }
}
