# Weather icon assets

These files are derived from the `fill` style in [`@meteocons/svg-static` 0.1.0](https://www.npmjs.com/package/@meteocons/svg-static), created by Bas Milius and distributed under the MIT License. See [LICENSE](LICENSE).

The files keep HA Deck's Home Assistant-facing names. Upstream names differ for the following conditions:

| HA Deck file | Meteocons source |
| --- | --- |
| `exceptional.svg` | `extreme.svg` |
| `lightning-rainy.svg` | `thunderstorms-rain.svg` |
| `lightning.svg` | `thunderstorms.svg` |
| `pouring.svg` | `extreme-rain.svg` |
| `snowy-rainy.svg` | `sleet.svg` |
| `windy-variant.svg` | `wind-alert.svg` |

Each SVG has an optically normalized `viewBox` with approximately 10% padding so the visible symbol fills HA Deck's square weather image consistently. Path and paint data are unchanged from upstream.
