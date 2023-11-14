# World binary file

* All values are little-endian and unsigned unless otherwise noted.
* All positions and dimensions are expressed as multiples of the map tile size
  (16x16 pixels) unless otherwise noted.
* All offsets are relative to the beginning of the file.

## World file header format

The world file header starts at file position 0.

| Offset | Size | Description |
| -- | -- | -- |
| `000000+00` | `2` | Count of maps in the world (`M`). |
| `000000+02` | `4*M` | Offsets for each map header. |
| `000000+02+4*M` | `2` | Count of boundaries (`B`). |
| `000000+04+4*M` | `4*B` | Offsets for each boundary. |

## Map header format

The map header offsets are found in the world file header.

| Offset | Size | Description |
| -- | -- | -- |
| `offset+00` | `2` | Signed X position of the map. |
| `offset+02` | `2` | Signed Y position of the map. |
| `offset+04` | `2` | Width of the map (`Mw`). |
| `offset+06` | `2` | Height of the map (`Mh`). |
| `offset+08` | `1` | Count of map tilesets (`T`) in the map. |
| `offset+09` | `4*T` | Offsets of the map tilesets. |
| `offset+09+4*T` | `1` | Count of entity tilesets (`U`) in the map. |
| `offset+0a+4*T` | `4*U` | Offsets of the entity tilesets. |
| `offset+0a+4*(T+U)` | `1` | Count of map tile layers (`L`) in the map. |
| `offset+0b+4*(T+U)` | `4*L` | Offsets of the map tile layers. |
| `offset+0b+4*(T+U+L)` | `1` | Layer index where entities should be rendered. |
| `offset+0c+4*(T+U+L)` | `2` | Count of entities in the map (`E`). |
| `offset+0e+4*(T+U+L)` | `10*E` | Entities in the map. |
| `offset+0e+4*(T+U+L)+10*E` | `2*E` | *X sorted*. |
| `offset+0e+4*(T+U+L)+12*E` | `2*E` | *X+W sorted*. |
| `offset+0e+4*(T+U+L)+14*E` | `2*E` | *Y sorted*. |
| `offset+0e+4*(T+U+L)+16*E` | `2*E` | *Y+H sorted*. |

Note that the sorted entity sequences  are specified as follows:

* *X sorted* are the entity indices of each entity in the map in ascending order
  of their position's X component.
* *X+W sorted* are the entity indices of each entity in the map in ascending
  order of the sum of their position's X component and their tile width.
* *Y sorted* are the entity indices of each entity in the map in ascending order
  of their position's Y component.
* *Y+H sorted* are the entity indices of each entity in the map in ascending
  order of the sum of their position's Y component and their tile height.

## Tileset format

Documented in the [tileset specification](tileset.md). Most importanly, it
contains:

* The width (`Tw`) and height (`Th`) of each tile in pixels.
* The count of columns (`Tc`) of the tileset. This must be inferred by
  dividing the pixel width of the tileset's source image by `Tw`.

## Map tile layer format

The map tile layer offsets are found in the map headers.

| Offset | Size | Description |
| -- | -- | -- |
| `offset+00` | `1` | Numerator of the layer parallax in the X dimension. |
| `offset+01` | `1` | Denominator of the layer parallax in the X dimension. |
| `offset+02` | `1` | Numerator of the layer parallax in the Y dimension. |
| `offset+03` | `1` | Denominator of the layer parallax in the Y dimension. |
| `offset+04` | `Mw*Mh` | Tiles listed by row from top-left to bottom-right. |
  
## Entity format

The following table specifies the byte sequence of entity data:

| Position | Size | Description |
| -- | -- | -- |
| `00` | `2` | X position of the entity in pixels. |
| `02` | `2` | Y position of the entity in pixels. |
| `04` | `2` | Entity tile ID (`Tid`). |
| `06` | `4` | Entity state. |

## Tiles

* The first 4 bits of the tile are the tileset index. This limits the count of
  tilesets attached to a map to 32, that being 16 for map tile tilesets and 16
  for entity tilesets.
* For map tiles, the remaining 12 bits are the tile ID (_Tid_). This limits the
  count of usable map tiles in a tileset to 4096.
* For entity tiles, the next 2 bits are the flip x and flip y attributes
  respectively. The remaining 10 bits are the tile ID (_Tid_). This limits the
  count of usable entity tiles in a tileset to 1024.

### Additional nodes on tiles

* __Zero values are always empty and unrendered.__
* The image that should be rendered for that tile will be a `Tw` by `Th` pixel
  rectangle at the (x,y) coordinates of the image referenced by the tileset
  where:
  > x = `Tw` * (`Tid` % `Tc`)  
  > y = `Th` * floor(`Tid` / `Tc`)
* All map tileset tiles must be 16 x 16 pixels.
* Entity tileset tiles may be any size.
* Map and entity layers may share tilesets. Consequently, the offset to a map
  tileset may appear in the list of entity tileset offsets, however, the tiles'
  tileset indices may differ between map and entity layers.

## Boundary format

The boundary offsets are found in the map file header.

| Offset | Size | Description |
| -- | -- | -- |
| `offset+00` | `1` | Flags for this boundary. |
| `offset+01` | `2` | Count of lines comprising this boundary (`S`). |
| `offset+03` | `8*S` | The points comprising the boundary lines. |

## Boundary flags

See source for details.

## Boundary point format

The following table specifies the byte sequence of entity data:

| Position | Size | Description |
| -- | -- | -- |
| `00` | `4` | Signed X position of the point in pixels. */
| `04` | `4` | Signed Y position of the point in pixels. */

Note that each point in the list of points comprising the boundary lines is
connected to the next point in the list.
