# Offline map and GPS architecture

The map, position source, and route planner should be three independent systems:

```text
                        +----------------------+
country pack ---------->+ tile source          |
(.mbtiles)              | visible tiles + LRU  +----+
                        +----------------------+    |
                                                    v
GPS / simulator ------->+ position provider    +--> map renderer --> Pip-Boy overlays
                        | latest validated fix  +    ^
                        +----------------------+    |
                                                    |
route graph ----------->+ route planner        +----+
(future)                | path + instructions  |
                        +----------------------+
```

Keeping these parts separate matters. GPS can provide a position with no map installed, a map can be browsed with no GPS hardware, and routing can be added later without replacing either one.

## Recommended first version: raster MBTiles

Use one raster [MBTiles 1.3](https://github.com/mapbox/mbtiles-spec/blob/master/1.3/spec.md) file per downloaded country or region. MBTiles is a SQLite container whose `tiles` table stores PNG/JPEG tile blobs by zoom, column, and row. It is a much better runtime input than raw `.osm` or `.osm.pbf` data: OSM extracts describe roads and features but do not contain styled pixels, labels, or a render order.

Raster tiles are the pragmatic first step for this SDL2 project:

- SQLite is the only new runtime dependency.
- `tile_data` can be decoded with `IMG_Load_RW` and uploaded as an SDL texture.
- A green color modulation, scanlines, markers, and other Fallout-style effects can be applied after decoding.
- The renderer only needs the tiles intersecting the 700 x 370 viewport.

Vector MBTiles can be considered later. They reduce some storage and allow deeper restyling, but require a Mapbox Vector Tile decoder, font shaping, symbol placement, road casing, and collision handling. That is effectively a second rendering engine.

### Tile lookup

For an XYZ zoom `z`, longitude `lon`, and latitude `lat`, clamp latitude to Web Mercator's useful range and calculate:

```text
n = 2^z
x = (lon + 180) / 360 * n
y = (1 - asinh(tan(lat * pi / 180)) / pi) / 2 * n
```

The MBTiles specification uses TMS rows, whose Y axis is reversed from XYZ:

```text
tile_row_tms = (2^z - 1) - tile_y_xyz
```

The engine should retain the fractional part of `x` and `y` for smooth pixel positioning. Clamp the viewport to the pack's `bounds` metadata instead of allowing empty space beyond the downloaded region.

### Runtime tile cache

Do not query, decode, and upload every tile on every frame. Use this flow:

1. Calculate the visible XYZ tile range after a pan or zoom change.
2. Look up each tile in an LRU cache keyed by `(pack_id, zoom, x, y)`.
3. Queue cache misses for a worker that performs SQLite reads and image decoding.
4. Create and destroy renderer-bound SDL textures on the rendering thread.
5. Cap cache memory by decoded bytes, not only by tile count.

Keep the generated grid currently in `MAP/map.c` as a fallback/debug tile source. It makes input, overlays, and GPS simulation testable when no country pack is installed.

## Country-pack download flow

The application should download finished packs, not scrape public map tiles and not render a country-scale PBF on the device.

The standard `tile.openstreetmap.org` service explicitly prohibits bulk/offline downloading in the [OpenStreetMap tile usage policy](https://operations.osmfoundation.org/policies/tiles/). Country extracts are available as source data from services such as [Geofabrik](https://download.geofabrik.de/), but those PBF files should be converted to styled MBTiles in a separate pack-building pipeline or on a server under your control.

A small signed or HTTPS-served manifest can drive the in-app picker:

```json
{
  "id": "us-midwest",
  "name": "United States — Midwest",
  "version": "2026-08-01",
  "format": "png",
  "min_zoom": 4,
  "max_zoom": 14,
  "bounds": [-104.1, 35.9, -80.5, 49.4],
  "bytes": 123456789,
  "sha256": "...",
  "download_url": "https://example.invalid/maps/us-midwest.mbtiles",
  "attribution": "© OpenStreetMap contributors"
}
```

Downloader requirements:

- Show the size before download and check free disk space.
- Download to `filename.part`, support HTTP range resume, and never expose a partial pack to the map engine.
- Verify SHA-256, open the SQLite file read-only, validate required metadata/tables, then rename atomically.
- Keep attribution and source/license metadata with the pack.
- Let users delete or update packs independently.
- Use regional packs where a full-country pack would be unreasonably large; storage grows rapidly with maximum zoom.

## Offline GPS

Receiving a GPS fix does not require internet access, but the device still needs a location source. Define a provider interface and keep platform details outside the map renderer:

```c
typedef struct {
    bool valid;
    double latitude;
    double longitude;
    double altitude_m;
    double speed_mps;
    double heading_degrees;
    double horizontal_accuracy_m;
    uint64_t timestamp_ms;
} GpsFix;

typedef struct {
    bool (*start)(void *context);
    bool (*poll)(void *context, GpsFix *fix);
    void (*stop)(void *context);
    void *context;
} PositionProvider;
```

Start with two providers:

1. A simulator that reads a trace file or moves a fix with development keys. This makes the full UI testable without hardware.
2. A serial NMEA provider for common USB/Bluetooth GPS receivers. On systems already running GPSD, its JSON client protocol is generally easier and more complete than parsing NMEA in every application; see the official [GPSD client guide](https://www.gpsd.io/client-howto.html).

Validate fixes before publishing them: coordinate range, finite numeric values, age, fix validity, and realistic accuracy. The UI can then support north-up/heading-up modes, a follow toggle, an accuracy indicator, breadcrumb trails, discovered locations, and Fallout-style quest markers.

## Routing is a later, separate feature

Showing the user's position is offline GPS. Turn-by-turn navigation additionally needs a routable road graph, snapping, a pathfinding engine, and maneuver generation. Do not put that work into the tile renderer. Add a `RoutePlanner` interface later and ship a routing graph beside each country pack or generate routes through a separately embedded engine.

## Suggested milestones

1. Keep the grid fallback, add a geographic viewport, and drive it with a simulated `GpsFix`.
2. Add read-only raster MBTiles, Web Mercator conversion, and an LRU tile cache.
3. Add the country-pack manifest, resumable downloader, validation, and pack manager UI.
4. Add serial NMEA and/or GPSD position providers plus follow/heading modes.
5. Add offline routing only after map browsing and live positioning are solid.

This order produces a useful offline map early and avoids coupling the hardest parts of the project together.
