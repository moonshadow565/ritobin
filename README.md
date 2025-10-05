Convert rito's .bin files to custom text format:
```
./ritobin2txt input.bin output.txt
```
 
Convert it back:
```
./txt2ritobin input.txt output.bin
```

 ritobin_cli.exe options
```
Usage: ritobin [options] input output

Positional arguments:
input                   input file or directory[Required]
output                  output file or directory

Optional arguments:
-h --help               show this help message and exit
-k --keep-hashed        do not run unhasher
-r --recursive          run on directory
-i --input-format       format of input file
-o --output-format      format of output file
-d --dir-hashes         directory containing hashes

Formats:
        - text
        - json
        - info
        - bin
```
 
 Custom text format example
 ```py
#PROP_text

# This is a comment

type: string = "PROP"
version: u32 = 2
linked: list[string] = {
  "DATA/Characters/Ashe/Ashe.bin"
}
entries: map[hash,embed] = {
  0xac080f05 = SkinCharacterDataProperties {
    skinClassification: u32 = 1
    metaDataTags: string = "race:human"
    skinAudioProperties: embed = skinAudioProperties {
      tagEventList: list[string] = {
        "Ashe"
      }
      bankUnits: list[embed] = {
        BankUnit {
          name: string = "Ashe_Base_SFX"
          bankPath: list[string] = {
            "ASSETS/Ashe_Base_SFX_audio.bnk"
            "ASSETS/Ashe_Base_SFX_events.bnk"
          }
        }
        BankUnit {
          name: string = "Ashe_Base_VO"
          bankPath: list[string] = {
            "ASSETS/Ashe_Base_VO_audio.bnk"
            "ASSETS/Ashe_Base_VO_events.bnk"
            "ASSETS/Ashe_Base_VO_audio.wpk"
          }
          voiceOver: bool = true
        }
      }
    }
    skinAnimationProperties: embed = skinAnimationProperties {
      animationGraphData: link = 0x2298a5c9
    }
    0x8bb2d7a4: list[string] = {
      "ASSETS/Characters/Ashe/HUD/Icons2D/Ashe_P_Debuff.dds"
      "ASSETS/Characters/Ashe/HUD/Icons2D/Chronokeeper_TimeStop.dds"
    }
    armorMaterial: string = "Flesh"
    mContextualActionData: link = "Characters/Ashe/CAC/Ashe_Base"
    iconCircle: option[string] = {
      "ASSETS/Characters/Ashe/HUD/Ashe_Circle.dds"
    }
    iconSquare: option[string] = {
      "ASSETS/Characters/Ashe/HUD/Ashe_Square.dds"
    }
  }
}
```

# Linux build instructions
Test on Ubuntu 24.04
```
sudo apt update
sudo apt install gcc-14 g++-14 cmake ninja-build git
git clone https://github.com/moonshadow565/ritobin
cmake -G Ninja -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14 -S ritobin -B ritobin-build
cmake --build ritobin-build
```
