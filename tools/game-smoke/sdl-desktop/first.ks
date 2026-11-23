*menu|Harbor Menu
[cm][nowait]
[eval exp="f.lamp=0"][eval exp="record('menu:' + f.lamp)"]
[locate x=32 y=32][link target=*enter]Enter the harbor[endlink]
[locate x=32 y=100][link target=*loadgame]Load the lamp checkpoint[endlink]
[s]

*enter|Harbor Entrance
[eval exp="f.lamp=42"]
[jump target=*checkpoint]

*checkpoint|Harbor Lamp
[cm][nowait]
[eval exp="record('checkpoint:' + f.lamp)"]
[locate x=32 y=32]The harbor lamp still shines across the water.
[locate x=32 y=100][link target=*savegame]Save the lamp checkpoint[endlink]
[locate x=32 y=168][link target=*exitgame]Leave the harbor[endlink]
[s]

*savegame
[eval exp="saveCheckpoint()"]
[jump target=*exitgame]

*loadgame
[load place=1]

*exitgame
[eval exp="record('exit')"][eval exp="System.exit()"]
[s]
