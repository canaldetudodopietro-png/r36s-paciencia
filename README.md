Notes
Thanks to Wes Cherry for writing the original Windows Solitaire that inspired this port. This is a from-scratch Klondike implementation in C with SDL2: every card, suit and font is drawn by code, and the bouncing cards victory animation is included.
Controls
Key	Action
Left Stick / D-Pad	Move cursor
A	Click (hold to drag cards, tap a card to send it to the foundations)
B	Smart move (foundation, otherwise another column)
X	Slow cursor (hold)
Y	Undo
L1	Switch Draw 1 / Draw 3
R1	Play every possible card to the foundations
Start	New deal (press twice)
Select	Quit (press twice)
Compile
1. Dependencies (Debian/Ubuntu, aarch64)
```
sudo apt install build-essential libsdl2-dev
```
2. Build
```
git clone <repository-url>
cd <repository>
make
```
