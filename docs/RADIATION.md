# Radiation model and display

Pipboy-3000 uses a Fallout 4-style accumulated-radiation model while exposing
the exact number for clarity.

## Gameplay model

Radiation ranges from `0` to `1000` and blocks the matching percentage of the
player's original maximum HP:

```text
effective_max_hp = max_hp * (1000 - radiation) / 1000
```

The integer result is rounded to the nearest HP. With a base maximum of 115:

| Radiation | Effective maximum HP |
| ---: | ---: |
| 0 | 115 |
| 100 | 104 |
| 300 | 81 |
| 500 | 58 |
| 900 | 12 |
| 1000 | 0 |

Gaining radiation immediately clamps current HP to the new effective maximum.
Removing radiation increases the amount that can be healed but does not grant
HP by itself. Stimpaks cannot heal above the effective maximum.

## Footer display

The STAT footer is divided into:

```text
HP current/effective + exact RAD | LEVEL + XP | AP current/maximum
```

The HP meter always represents the original maximum-HP width:

- solid green: current HP;
- dark green: healable missing HP;
- orange with dark stripes: HP capacity blocked by radiation.

The exact orange `RAD n` label is hidden at zero radiation. The middle footer
retains the level and XP readout plus a progress meter.

## Future extensions

Ambient exposure rate (`RAD/SEC`), Geiger-counter click frequency, Rad-X
resistance, radiation-sickness thresholds, and death handling at 1000 RADS are
separate systems and can be layered onto this accumulated-dose model later.

Until death/respawn handling exists, the `Z` development control is capped at
900 RADS. `C` restores test HP, radiation, Stimpaks, and RadAway so UI
experiments cannot leave the prototype in a dead-but-interactive or depleted
state. Gameplay commands can still model
the full 1000-RAD lethal threshold independently of that debug guardrail.
