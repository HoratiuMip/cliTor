//# TUFILIN - your includes here.

#include <bridge.hpp>
//# TUFILIN - specify the junction namespace and junction name.
JUNCTION_HEADER( , "" )

//# TUFILIN - the dock class.
class  : public Dock {

//# SAMPLE - get a strong lock on the bridge's IMM and this dock's UIX pack.
//#        - this macro shall be used outside the UIX related functions, such as
//#          loading a texture from a camera capture.
    void process_data() {
        JUNCTION_DOCK_WITH_BRIDGE_IMM_AND_UIX_PACK {
            imm->do_something_with( uix_pack->data );
        }
    }

#pragma region UIX
//# TUFILIN - optional, structure used by this dock's UIX.
    struct UIX_pack : ::Dock::UIX_pack {
        virtual ~UIX_pack() override = default;
    };

//# TUFILIN - optional, specify that the dock cannot be removed from the UIX, i.e. it won't have a close button.
    JUNCTION_DOCK_IS_UIX_PERSISTENT

//# TUFILIN - optional, the function called when the UIX begins, or the dock is installed.
//#         - return a shared reference to your UIX pack structure. It will be passed to your UIX frame callback.
    JUNCTION_DOCK_UIX_BEGIN_FNC_SIG {
        return std::make_shared< UIX_pack >();
    }
//# TUFILIN - optional, the function called each frame of the UIX.
//#         - all UIX related memory is guaranteed to be valid inside this function.
//#         - a reference to your UIX pack is passed through this function's argument.
    JUNCTION_DOCK_UIX_FRAME_FNC_SIG {
        return OK;
    }
//# TUFILIN - optional, the function called when the UIX ends, or the dock is uninstalled.
    JUNCTION_DOCK_UIX_END_FNC_SIG {

    }
#pragma endregion UIX

};

class Proxy : public ::Proxy {
public:
//# REQUIRED - name of the junction through its proxy.
    JUNCTION_PROXY_GET_NAME

//# TUFILIN - optional, the function called after the bridge started.
    JUNCTION_PROXY_WAKE_FNC_SIG {
    }

//# TUFILIN - required, the command interpreter of your proxy.
    JUNCTION_PROXY_PASS_FNC_SIG{
        return OK;
    }
};
JUNCTION_PROXY_INSTALL( Proxy )
JUNCTION_FOOTER
