# Book of Knowledge
Adds a `Librarian` that accepts `Book's of Knowledge` to increase base attributes.

## Install
- Clone this repo to your modules folder or download
  and place in your modules folder in your
  Azerothcore source
- Add the BookofKnowledgeUI addon to your addons
  folder.
- Spawn in the librarian where you'd like players to
  turn in their books (npc_id: 441153).
- Enable the module in the conf file
- Adjust additional setting in the conf to your
  preferences.
- The Book of Knowledge is added to the database
  during the compile
- You can chnage the Book of Knowledge and the
  Librarian ID's in the SQL files 
  if there are conflicts with ID's. SQL file is here
  \mod-bookofknowledge\data\sql\db-world\base

## Configuration & Settings

This module allows you to configure how books are acquired and precisely limit which stats players can upgrade and how much.

### General Settings
- **`Book of Knowledge.Enable`**: Toggles the entire module on (`1`) or off (`0`). 
- **`Book of Knowledge.DisablePvP`**: If enabled (`1`), extra attributes are nullified while the player is engaged in PvP combat.
- **`Book of Knowledge.ResetCost`**: The cost (in copper) to reset all spent Knowledge Points at the Librarian. Default is `2500000` (250 Gold).

### Item Acquisition
By default, the *Book of Knowledge* can be acquired passively through typical gameplay loops:
- **`Book.Loot.Enable` / `Chance`**: Allows the item to drop globally from any creature loot and adjust the drop chance, default chance is 5%.
- **`Book.Quest.Enable` / `Chance`**: Grants a chance to receive a book alongside regular quest rewards, default chance is 10%.

### Stat Configuration & Limits
Every individual stat can be enabled/disabled from showing up in the UI, and each stat has a max cap.

**Available Stats:**
* **Primary Attributes:** Strength, Agility, Stamina, Intellect
* **Offensive Power:** Attack Power, Spell Power
* **Defensive Ratings:** Block, Dodge, Parry, Defense
* **Combat Ratings:** Melee/Ranged Hit, Crit, Haste, and Expertise
* **Spell Ratings:** Spell Hit, Spell Crit, Spell Haste

### Stat Rates
All stat rates should be the same as in-game rates and caculated based on player level

**Example Configuration:**
```ini
# Completely disable the ability to add points to 'Dodge' rating
Book of Knowledge.Stat.Dodge = 0

# Limit the maximum amount of Stamina a player can add points to 50
Book of Knowledge.Max.Stamina = 50

## Credits
- AnchyDev - The original author of the Attriboost module
  which is what this module is inspired from
- Foe - for his idea for using auras as attributes
- Anyone else - For anyone else who helped with the initial Attriboost module
