# One Button Rythm
One button rythm is a rythm game with only one button.  
It's meant to be a simple game, simple to play, simple to make maps (Edit: it's no longer simple).

Supports exporting / importing zipped maps  
Supports theming via botchered css  
  
### Editing features:  
    custom textures or sound effects  
    animation support  
    multiple BPM per map
    
### Editor Controls:
    z: place note
    x: remove note
    c: snap
    a: place note on snap
    e: add bars
    q: remove bars
    d: add timing segment
    f: add timing segment
    

![screenshot1](https://user-images.githubusercontent.com/50572621/200126707-b87e1d8c-2509-46ae-95e3-245c71b0a8ca.png)

![screenshot2](https://user-images.githubusercontent.com/50572621/200126708-af544e02-0b16-495c-9963-34a20919d183.png)

![screenshot3](https://user-images.githubusercontent.com/50572621/200126714-df3384a3-d4d5-4f12-8fbd-95c8a72fcbd3.png)


Current doesn't build with MSVC, does work with gcc and mingw 

### to compile

    git clone https://github.com/Big-Brain-Boys-Games/onebuttonRythm.git --recursive
    cd onebuttonRythm
    cmake .
    make
