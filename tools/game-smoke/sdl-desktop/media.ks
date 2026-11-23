*harbor|Harbor Broadcast
[cm][nowait]
[eval exp="record('ready')"]
[playbgm storage=harbor.wav loop=true]
[playse storage=bell.wav buf=0 loop=false]
[wait time=450]
[video slot=0 left=224 top=176 width=192 height=128 visible=true]
[playvideo slot=0 storage=arrival.mp4]
[eval exp="record('natural:start')"]
[wv slot=0 canskip=false]
[eval exp="record('natural:end')"]
[locate x=32 y=32][link target=*second]Watch the second broadcast[endlink]
[s]

*second|Second Broadcast
[cm][nowait]
[video slot=0 left=224 top=176 width=192 height=128 visible=true]
[playvideo slot=0 storage=departure.mp4]
[eval exp="record('skip:start')"]
[wv slot=0 canskip=true]
[stopbgm]
[eval exp="record('skip:end')"]
[locate x=32 y=32][link target=*exit]Leave the harbor[endlink]
[s]

*exit
[eval exp="record('exit')"][eval exp="System.exit()"]
[s]
