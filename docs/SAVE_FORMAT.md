# Pip-Boy save format

Runtime state is stored in `saves/player.json`. The save is intentionally
separate from the CSV item definitions: CSV files define items, while the save
stores mutable quantities and flags by stable item ID.

## Version 1

```json
{
  "version": 1,
  "player": {
    "health": 105,
    "max_health": 115,
    "ap": 90,
    "max_ap": 90,
    "radiation": 70,
    "max_radiation": 1000,
    "level": 3,
    "current_xp": 10,
    "xp_for_next_level": 200
  },
  "inventory": [
    {
      "id": "weapon_hunting_rifle",
      "quantity": 1,
      "equipped": true,
      "favorite": true
    },
    {
      "id": "aid_stimpak",
      "quantity": 3,
      "equipped": false,
      "favorite": false
    }
  ]
}
```

Unknown item IDs are ignored so content can be removed without making a save
unreadable. Missing fields retain their definition/default value. Numeric
fields are clamped to valid ranges, and only one weapon and one apparel item
may remain equipped after loading.

Effective maximum HP is derived from `max_health`, `radiation`, and
`max_radiation`; it is not serialized separately. Loaded current HP is clamped
to that derived cap.

## Atomic writes and recovery

Saves are written to `player.json.tmp` first. A successful write moves the
previous primary file to `player.json.backup`, then installs the temporary file
as the new primary. If the primary JSON cannot be parsed or has an unsupported
version, the loader attempts the backup. A recovered backup is immediately
written back as the new primary.

If both files are invalid, gameplay can continue with defaults but persistence
is disabled for that session so corrupt evidence is not overwritten.

## Migration policy

Future incompatible schema changes must increment `version`. Loaders should
retain explicit migration code for older supported versions rather than
silently interpreting them as the newest structure.
