Convert rito's .bin files to custom text format:
```
./ritobin2txt input.bin output.txt
```
 
Convert it back:
```
./txt2ritobin input.txt output.bin
```
 
 Sample output
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
