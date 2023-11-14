# Tileset binary file

* All values are little-endian and unsigned unless otherwise noted.
* All strings are NULL-terminated.
* All offsets are relative to the beginning of the file.

## Tileset file header format

The tileset file header starts at file position 0.

| Offset | Size | Description |
| -- | -- | -- |
| `000000+00` | `2` | Count of tiles. |
| `000000+02` | `2` | Width of each tile in pixels. |
| `000000+04` | `2` | Height of each tile in pixels. |
| `000000+06` | `4` | Offset of the image source string. |
| `000000+0a` | `4` | Offset of the library name string. |
| `000000+0e` | `2` | Count of tile data entries (`D`) in this tileset. |
| `000000+10` | `4*D` | Offsets of the tile data entries. |

## Tile data entry format

Tile data entry offsets are found in the tileset file header.

| Offset | Size | Description |
| -- | -- | -- |
| `offset+00` | `2` | Index of the tile associated with this data. |
| `offset+02` | `4` | Name of this tile. |
| `offset+06` | `4` | Offset of the library name string. |
| `offset+0a` | `1` | Count of collision box type headers (`T`) for this tile. |
| `offset+0b` | `4*T` | Offsets of the collision box type headers. |
| `offset+0b+4*T` | `1` | Count of tiles (`A`) in the animation. |
| `offset+0c+4*T` | `6*A` | Animation tiles. |

## Collision box type header format

The offsets of the collision box type headers are found in the tile data
entries.

| Offset | Size | Description |
| -- | -- | -- |
| `offset+00` | `4` | Collision box type. |
| `offset+04` | `2` | Count of collision box lists (`L`) for this type. |
| `offset+06` | `4*L` | Offsets of the collision box lists. |

## Collision box list format

The offsets of the collision box lists are found in the collision box type
headers.

| Offset | Size | Description |
| -- | -- | -- |
| `offset+00` | `4` | Collision box list name. |
| `offset+04` | `2` | Count of collision boxes (`B`) for this list. |
| `offset+06` | `8*B` | Collision boxes for this list. |

## Collision box format

The following table specifies the byte sequence describing a collision box.

| Position | Size | Description |
| -- | -- | -- |
| `00` | `2` | X position of the top-left corner of the box in pixels. |
| `02` | `2` | Y position of the top-left corner of the box in pixels. |
| `04` | `2` | Width of the box in pixels. |
| `06` | `2` | Height of the box in pixels. |

## Animation tile format

The following table specifies the byte sequence describing an animation tile.

| Position | Size | Description |
| -- | -- | -- |
| `00` | `2` | Tile ID. |
| `02` | `2` | Duration in frames. |
