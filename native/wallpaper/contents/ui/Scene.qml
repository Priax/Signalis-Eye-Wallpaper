import QtQuick
import QtQuick3D
import SignalisEye

Item {
    id: root

    property alias controller: ctrl
    property alias cursor: ctrl.cursor
    property alias paused: ctrl.paused
    property alias fps: ctrl.fps
    property alias idleFps: ctrl.idleFps
    property real brightness: 0
    property bool showRaw: false

    EyeController { id: ctrl }

    component Tex: Texture {
        minFilter: Texture.Nearest
        magFilter: Texture.Nearest
        mipFilter: Texture.None
        generateMipmaps: false
    }

    component Unlit: CustomMaterial {
        property TextureInput map: TextureInput {}
        property real cutoff: 0
        property real blended: 0
        property real sway: 0
        property real time: 0
        property real holeMode: 0
        property real holeRadius: 30.5
        property real holeCos: 0.94943
        property vector3d holeCenter
        property vector3d holeDir
        shadingMode: CustomMaterial.Unshaded
        vertexShader: "../shaders/unlit.vert"
        fragmentShader: "../shaders/unlit.frag"
    }
    component Transparent: Unlit {
        blended: 1
        sourceBlend: CustomMaterial.SrcAlpha
        destinationBlend: CustomMaterial.OneMinusSrcAlpha
    }

    component Mesh: Node {
        property alias source: model.source
        property alias materials: model.materials
        property alias depthBias: model.depthBias
        Model { id: model; scale: Qt.vector3d(-1, 1, 1) }
    }

    View3D {
        id: view
        anchors.fill: parent
        renderMode: View3D.Offscreen
        explicitTextureWidth: 640
        explicitTextureHeight: 360
        camera: camera

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Color
            clearColor: "black"
            antialiasingMode: SceneEnvironment.NoAA
            tonemapMode: SceneEnvironment.TonemapModeNone
        }

        Tex { id: texWhite; source: "../textures/eyecloseup_eyeWhite.png" }
        Tex { id: texPupil; source: "../textures/eyecloseup_pupil.png" }
        Tex { id: texShine; source: "../textures/eyecloseup_shine.png" }
        Tex { id: texFace; source: "../textures/eyecloseup_face.png" }
        Tex { id: texLid; source: "../textures/eyecloseup_lid.png" }
        Tex { id: texHair; source: "../textures/eyecloseup_hair.png" }
        Tex { id: texGlow; source: "../textures/circular_light_blue.png" }

        Node {
            scale: Qt.vector3d(1, 1, -1)

            Node {
                position: ctrl.eyePosition
                rotation: Qt.quaternion(0.7071068, 0.7071068, 0, 0)
                scale: Qt.vector3d(10, 10, 10)

                Node { id: lightHolder; position: Qt.vector3d(0.145, -5.2, -0.81) }

                Node {
                    rotation: ctrl.lookRotation

                    Mesh {
                        rotation: Qt.quaternion(0, 0, 0.7071068, -0.7071068)
                        source: "../models/EyePupil_noArmature_1.mesh"
                        materials: CustomMaterial {
                            property TextureInput map: TextureInput { texture: texPupil }
                            property vector3d lightPos: lightHolder.scenePosition
                            property real range: 43.94
                            property real intensity: 94.2
                            property real diffuse: 0.233472
                            property real ndlFloor: 0.25
                            shadingMode: CustomMaterial.Unshaded
                            vertexShader: "../shaders/pupil.vert"
                            fragmentShader: "../shaders/pupil.frag"
                        }
                    }
                    Mesh {
                        rotation: Qt.quaternion(0, 0, 0.7071068, -0.7071068)
                        source: "../models/EyePupil_noArmature_0.mesh"
                        materials: Transparent { map.texture: texPupil }
                        depthBias: 1000
                    }
                    Mesh {
                        rotation: Qt.quaternion(0, 0, 0.7071068, -0.7071068)
                        source: "../models/EyePupil_Trans_0.mesh"
                        materials: Transparent {
                            map.texture: texPupil
                            holeMode: 1
                            holeCenter: ctrl.holeCenter
                            holeDir: ctrl.holeDir
                        }
                    }
                }

                Mesh {
                    source: "../models/EyeShine_0.mesh"
                    materials: Transparent { map.texture: texShine }
                    depthBias: 500
                }
                Mesh {
                    position: Qt.vector3d(0, 0.04, 0)
                    source: "../models/EyeWhite_0.mesh"
                    materials: Unlit {
                        map.texture: texWhite
                        holeMode: 1
                        holeCenter: ctrl.holeCenter
                        holeDir: ctrl.holeDir
                    }
                }
                Mesh {
                    position: Qt.vector3d(0, -0.091, 0)
                    source: "../models/FacePlane_0.mesh"
                    materials: Transparent { map.texture: texFace }
                }
                Mesh {
                    position: Qt.vector3d(0.2, -0.32, 0)
                    source: "../models/HairPlane_0.mesh"
                    materials: Transparent {
                        map.texture: texHair
                        sway: 0.5
                        time: ctrl.hairTime
                        cullMode: Material.NoCulling
                    }
                    depthBias: -1000
                }
                Mesh {
                    position: Qt.vector3d(0, 1.64, 0)
                    source: "../models/EyeWhiteBack_0.mesh"
                    materials: Unlit {
                        map.texture: texWhite
                        holeMode: 2
                        holeCenter: ctrl.holeCenter
                        holeDir: ctrl.holeDir
                    }
                }

                Mesh {
                    visible: ctrl.lidDownTrans
                    position: Qt.vector3d(1.215, -2.930002, -1.53)
                    source: "../models/FaceLid_Down_Transition_0.mesh"
                    materials: Unlit { map.texture: texLid; cutoff: 0.5 }
                }
                Mesh {
                    visible: ctrl.lidDownLook
                    position: Qt.vector3d(1.33, -2.930001, -1.52)
                    source: "../models/FaceLid_Down_0.mesh"
                    materials: Unlit { map.texture: texLid; cutoff: 0.5 }
                }
                Mesh {
                    visible: ctrl.lidHalf
                    position: Qt.vector3d(1.215, -2.93, -1.48)
                    source: "../models/FaceLid_Half_0.mesh"
                    materials: Unlit { map.texture: texLid; cutoff: 0.5 }
                }
                Mesh {
                    visible: ctrl.lidClosed
                    position: Qt.vector3d(1.215, -2.93, -1.54)
                    source: "../models/FaceLid_0.mesh"
                    materials: Unlit { map.texture: texLid; cutoff: 0.5 }
                }
            }

            Model {
                source: "#Rectangle"
                position: Qt.vector3d(-0.30969208, 12.18123, 64.67762)
                scale: Qt.vector3d(1.28, 0.3079239, 1)
                depthBias: -5000
                materials: CustomMaterial {
                    property TextureInput map: TextureInput { texture: texGlow }
                    property real vMin: 0.144656
                    shadingMode: CustomMaterial.Unshaded
                    cullMode: Material.NoCulling
                    sourceBlend: CustomMaterial.One
                    destinationBlend: CustomMaterial.OneMinusSrcColor
                    vertexShader: "../shaders/screen.vert"
                    fragmentShader: "../shaders/screen.frag"
                }
            }

            Node {
                position: Qt.vector3d(-3.600006, 13.9, 26.6)
                rotation: Qt.quaternion(0.98645544, 0.14849833, 0.058743317, -0.037459098)
                PerspectiveCamera {
                    id: camera
                    scale: Qt.vector3d(1, 1, -1)
                    fieldOfView: 46.9
                    fieldOfViewOrientation: PerspectiveCamera.Vertical
                    clipNear: 0.3
                    clipFar: 1000
                }
            }
        }
    }

    ShaderEffect {
        visible: !root.showRaw
        anchors.fill: parent
        property var source: view
        property real falloff: ctrl.falloff
        property real brightness: root.brightness
        property real viewWidth: width
        property real viewHeight: height
        fragmentShader: "../shaders/vignette.frag.qsb"
    }
}
