import QtQuick
import QtQuick3D
CustomMaterial {
    id: material
    required property var part
    required property var avatarColors
    property bool isHead: part.head
    property color skinTone: avatarColors[0]
    property color mouthTone: avatarColors[2]
    property color irisTone: avatarColors[3]
    property color eyebrowTone: avatarColors[4]
    property color eyeShadowTone: avatarColors[5]
    property color facialHairTone: avatarColors[6]
    property color featureOne: avatarColors[7]
    property color featureTwo: avatarColors[8]
    property color customOne: part.customColors[0]
    property color customTwo: part.customColors[1]
    property color customThree: part.customColors[2]
    property int channel0: part.uvChannels[0]
    property int channel1: part.uvChannels[1]
    property int channel2: part.uvChannels[2]
    property int channel3: part.uvChannels[3]
    property int channel4: part.uvChannels[4]
    property int channel5: part.uvChannels[5]
    property TextureInput avatarMap0: TextureInput { texture: Texture { textureData: material.part.textures[0]; minFilter: Texture.Linear; magFilter: Texture.Linear } }
    property TextureInput avatarMap1: TextureInput { texture: Texture { textureData: material.part.textures[1]; minFilter: Texture.Linear; magFilter: Texture.Linear } }
    property TextureInput avatarMap2: TextureInput { texture: Texture { textureData: material.part.textures[2]; minFilter: Texture.Linear; magFilter: Texture.Linear } }
    property TextureInput avatarMap3: TextureInput { texture: Texture { textureData: material.part.textures[3]; minFilter: Texture.Linear; magFilter: Texture.Linear } }
    property TextureInput avatarMap4: TextureInput { texture: Texture { textureData: material.part.textures[4]; minFilter: Texture.Linear; magFilter: Texture.Linear } }
    property TextureInput avatarMap5: TextureInput { texture: Texture { textureData: material.part.textures[5]; minFilter: Texture.Linear; magFilter: Texture.Linear } }
    shadingMode: CustomMaterial.Shaded
    cullMode: Material.NoCulling
    vertexShader: "qrc:/app/shaders/avatar.vert"
    fragmentShader: "qrc:/app/shaders/avatar.frag"
}
