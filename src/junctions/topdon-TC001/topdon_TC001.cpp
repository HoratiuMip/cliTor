#include <opencv2/opencv.hpp>

#include <bridge.hpp>
JUNCTION_HEADER( topdon_TC001, "topdon-TC001" )

constexpr int   FRAME_WIDTH    = 256;
constexpr int   FRAME_HEIGHT   = 192;

class Topdon_TC001 : public Dock {
public:
    Topdon_TC001( void ) = default;

    Topdon_TC001( 
        IN   std::string   dev_ 
    ) {
        this->open( std::move( dev_ ) );
    }

protected:
    std::string        _dev       = {};
    cv::VideoCapture   _capt      = {};
    std::jthread       _capt_th   = {};
    rgh::imm::Tex      _tex       = {};

protected:
    void _process_frames( std::stop_token stop_tok_ ) {
        while( not stop_tok_.stop_requested() ) {
            std::this_thread::sleep_for( std::chrono::seconds{ 1 } );

        }
    }

public:
    status_t open( 
        IN   std::string    dev_ 
    ) {
        _dev = std::move( dev_ );

        _capt.open( _dev, cv::CAP_V4L2 );
        ASSERT_OR( _capt.isOpened() ) return ERR_OPEN;

        _capt_th = std::jthread{ &Topdon_TC001::_process_frames, this };
        return OK;
    }

    status_t close( void ) {
        _capt_th.request_stop(); ASSERT_AND( _capt_th.joinable() ) _capt_th.join();
        _capt.release();
    }

    cv::Mat rgbaVisual;

public:
    virtual status_t dock_uix_frame( 
        IN   const dock_uix_frame_args_t&   args_
    ) override {
        cv::Mat frame; _capt >> frame;  if( frame.empty() ) { spdlog::error( "no frame" ); return OK; }
        cv::Mat visualTopHalf = frame(cv::Rect(0, 0, FRAME_WIDTH, FRAME_HEIGHT));

        cv::cvtColor(visualTopHalf, rgbaVisual, cv::COLOR_BGR2RGBA);
        
       static bool uploade = false;
        if( not uploade ) {
            BridgE.uix_imm_weak()->push_payload( { rgh::Immersive::payload_t::Verb_TexUpld, rgh::Immersive::payload_t::tex_upld_t{ 
                .invk = _tex, .pxls = rgh::hvec_weak_ptr_t{ rgbaVisual.data }, .w=FRAME_WIDTH,.h=FRAME_HEIGHT,.prms={}} } );
            uploade = true;
        }
            BridgE.uix_imm_weak()->push_payload( { rgh::Immersive::payload_t::Verb_TexReld, rgh::Immersive::payload_t::tex_reld_t{ 
                .invk = _tex, .pxls = rgh::hvec_weak_ptr_t{ rgbaVisual.data }, .w=FRAME_WIDTH,.h=FRAME_HEIGHT}} );
        ImGui::Image( (ImTextureID)_tex.get(), ImVec2(512, 384));
        

        return OK;
    }

};

class Proxy : public ::Proxy {
public:
    JUNCTION_PROXY_GET_NAME

    virtual void proxy_wake( void ) override {
        auto dock = rgh::HVec< Topdon_TC001 >::make();
        dock->open( "/dev/video2" ); 
        BridgE.install_dock( "topdon-TC001", dock );
    }
};
JUNCTION_PROXY_INSTALL( Proxy )
JUNCTION_FOOTER
