#include <opencv2/opencv.hpp>

#include <rgh/gep/tempo.hpp>
#include <rgh/osp/IO_utils.hpp>
#include <rgh/osp/imm_widgets.hpp>

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
    std::string          _dev         = {};
    cv::VideoCapture     _capt        = {};
    std::jthread         _capt_th     = {};
    rgh::io::COM_ports   _com_ports   = { rgh::DispenserMode_Lock, { .config = { .filter = rgh::io::COM_PORT_FILTER_VIDEO } } };  
    
public:
    struct mM_tmp_t {
        float         mtmp   = 0;
        float         Mtmp   = 0;
        cv::Point2i   mloc   = {};
        cv::Point2i   Mloc   = {};
    };

    struct process_result_t {
        std::atomic< mM_tmp_t >   mM_tmp   = {};
    } proc_res;

    inline static constexpr std::pair< int, int >   COLORMAPS[]   = {
        { cv::COLORMAP_PLASMA,  ImPlotColormap_Plasma },
        { cv::COLORMAP_BONE,    ImPlotColormap_Greys },
        { cv::COLORMAP_JET,     ImPlotColormap_Jet },
        { cv::COLORMAP_COOL,    ImPlotColormap_Cool },
        { cv::COLORMAP_VIRIDIS, ImPlotColormap_Viridis }
    };
    
public:
    rgh::Ticker_lap< std::chrono::steady_clock >   last_frame_ticker   = { rgh::ticker_epoch_init_t{} };

protected:
    void _process_frames( std::stop_token stop_tok_ ) {
        while( not stop_tok_.stop_requested() ) {
            cv::Mat frame; _capt >> frame;
            ASSERT_OR( not frame.empty() ) {
                std::this_thread::sleep_for( std::chrono::seconds{ 1 } );
                continue;
            }
            ASSERT_OR( frame.cols == FRAME_WIDTH and frame.rows == FRAME_HEIGHT*2 ) {
                continue;
            }

            last_frame_ticker.lap< float >();

            cv::Mat bot_half = frame( cv::Rect( 0, FRAME_HEIGHT, FRAME_WIDTH, FRAME_HEIGHT ) );
            if( not bot_half.isContinuous() ) bot_half = bot_half.clone();
            bot_half = cv::Mat( FRAME_HEIGHT, FRAME_WIDTH, CV_16U, bot_half.data, bot_half.step );

            mM_tmp_t mM_tmp = {};
            double mtmpd = 0, Mtmpd = 0;

            cv::minMaxLoc( bot_half, &mtmpd, &Mtmpd, &mM_tmp.mloc, &mM_tmp.Mloc );
            mM_tmp.mtmp = static_cast< float >( mtmpd / 64.0 - 273.15 );
            mM_tmp.Mtmp = static_cast< float >( Mtmpd / 64.0 - 273.15 );

            proc_res.mM_tmp.store( mM_tmp, std::memory_order_relaxed );

            JUNCTION_DOCK_WITH_BRIDGE_IMM_AND_UIX_PACK( UIX_pack ) {
                auto& top_half = uix_pack->rgba_top_half;
                cv::cvtColor( frame( cv::Rect{ 0, 0, FRAME_WIDTH, FRAME_HEIGHT } ), top_half, cv::COLOR_YUV2RGBA_YUYV );

                cv::Mat gray_ch; cv::extractChannel( top_half, gray_ch, 0x0 );

                cv::applyColorMap( gray_ch, top_half, COLORMAPS[ uix_pack->colormap.load( std::memory_order_relaxed ) ].first );
                cv::cvtColor( top_half, top_half, cv::COLOR_BGR2RGBA );

                imm->push_payload( { rgh::Immersive::payload_t::Verb_TexReld, 
                    rgh::Immersive::payload_t::tex_reld_t{ 
                        .ref = uix_pack->tex, 
                        .pxls = rgh::hvec_weak_ptr_t{ top_half.data }, .w=FRAME_WIDTH,.h=FRAME_HEIGHT
                    }
                } );
            }
        }
    }

public:
    status_t open( 
        IN   std::string    dev_ 
    ) {
        ASSERT_OR( not _capt.isOpened() ) close();

        _dev = std::move( dev_ );

        _capt.open( _dev, cv::CAP_ANY );
        ASSERT_OR( _capt.isOpened() ) return ERR_OPEN;
        _capt.set( cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc( 'Y','U','Y','V' ) );
        _capt.set( cv::CAP_PROP_FRAME_WIDTH, FRAME_WIDTH );
        _capt.set( cv::CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT*2 );
        _capt.set( cv::CAP_PROP_CONVERT_RGB, 0 );

        _capt_th = std::jthread{ &Topdon_TC001::_process_frames, this };
        return OK;
    }

    status_t close( void ) {
        _capt_th.request_stop(); ASSERT_AND( _capt_th.joinable() ) _capt_th.join();
        _capt.release();
        _dev.clear();

        return OK;
    }

public:
    struct UIX_pack : Dock::UIX_pack {
        ~UIX_pack( void ) override = default;

        rgh::imm::Tex                 tex             = {};
        cv::Mat                       rgba_top_half   = {};
        std::atomic< int >            colormap        = 0x0;
        rgh::imm_widgets::COM_ports   com_ports;  
    };

    JUNCTION_DOCK_UIX_BEGIN_FNC_SIG {
        auto pack = std::make_shared< UIX_pack >();

        pack->com_ports.bind( _com_ports );

        BridgE.uix_imm_weak()->push_payload( { rgh::Immersive::payload_t::Verb_TexUpld, 
            rgh::Immersive::payload_t::tex_upld_t{ 
                .ref = pack->tex,
                .pxls = nullptr, .w = FRAME_WIDTH, .h = FRAME_HEIGHT, .prms = {}
            } 
        } );
        return std::move( pack );
    }

    JUNCTION_DOCK_UIX_FRAME_FNC_SIG {
        JUNCTION_DOCK_UIX_REINTR_PACK( UIX_pack );

        ImGui::SeparatorText( "Available cameras" ); {
            auto ports = _com_ports.watch();
            auto [ port, selected_now, rescan ] = pack->com_ports.imm_frame( ports, "Scan for cameras", "No cameras found." );

            if( selected_now ) BridgE.push( [ this, port ] { open( port->id ); } );
        }

        auto mM_tmp = proc_res.mM_tmp.load( std::memory_order_relaxed );

        ImGui::SeparatorText( "Video feed" ); ImGui::SeparatorEx( ImGuiSeparatorFlags_Vertical ); ImGui::SameLine();

        if( ImGui::BeginTable( "##vid-feed-tbl", 3, ImGuiTableFlags_Borders ) ) {
            ImGui::TableSetupColumn( "##vid-feed", ImGuiTableColumnFlags_WidthFixed );
            ImGui::TableSetupColumn( "##vid-data", ImGuiTableColumnFlags_WidthFixed );
            ImGui::TableSetupColumn( "##vid-mrks", ImGuiTableColumnFlags_WidthStretch );

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
                auto feed_pos = rgh::Immersive::here();
            
                ImGui::Image( ( ImTextureID )pack->tex.get(), ImVec2( FRAME_WIDTH*2, FRAME_HEIGHT*2 ) );
                ImGui::SameLine();
                rgh::Immersive::chpt_here();

                if( auto scrl = rgh::Immersive::io().MouseWheel; ImGui::IsItemHovered() and scrl != 0 ) {
                    auto colormap = pack->colormap.load( std::memory_order_relaxed );

                    if( scrl > 0 ) pack->colormap.store( (colormap + 1) % std::size( COLORMAPS ) );
                    else pack->colormap.store( colormap-1 < 0 ? std::size( COLORMAPS )-1 : colormap-1 );
                }

                if( last_frame_ticker.peek_lap< float >() >= 3 ) {
                    rgh::Immersive::movxy( feed_pos, { FRAME_WIDTH - 36, FRAME_HEIGHT - 36 } );
                    ImSpinner::SpinnerAngTriple(
                        "##vid-feed-to", 24, 30, 36, 3, ImSpinner::white, ImSpinner::red, ImSpinner::white, 3
                    );
                    rgh::Immersive::chpt_return();
                }

                auto colormap = COLORMAPS[ pack->colormap.load( std::memory_order_relaxed ) ];
                ImPlot::ColormapScale( 
                    "##data-scl", mM_tmp.mtmp, mM_tmp.Mtmp, { 0, FRAME_HEIGHT*2 }, "%.0f", 
                    ImPlotColormapScaleFlags_NoLabel | ( colormap.second == ImPlotColormap_Greys ? ImPlotColormapScaleFlags_Invert : 0 ),
                    colormap.second
                );

            ImGui::TableNextColumn();
                ImGui::BulletText( "Min:" ); ImGui::SameLine();
                rgh::Immersive::scale_font( 1.5 );
                    ImGui::SetNextItemWidth( 120 );
                    ImGui::DragFloat( "##min-tmp", &mM_tmp.mtmp, 0, 0, 0, "%.2f°C", ImGuiSliderFlags_NoInput );
                rgh::Immersive::scale_font();
                ImGui::BulletText( "Max:" ); ImGui::SameLine();
                rgh::Immersive::scale_font( 1.5 );
                    ImGui::SetNextItemWidth( 120 );
                    ImGui::DragFloat( "##max-tmp", &mM_tmp.Mtmp, 0, 0, 0, "%.2f°C", ImGuiSliderFlags_NoInput );
                rgh::Immersive::scale_font();

                ImGui::BulletText( "Abs diff: %.2f°C", mM_tmp.Mtmp - mM_tmp.mtmp );

            ImGui::EndTable();
        }
        
        return OK;
    }

};

class Proxy : public ::Proxy {
public:
    int   next_dock_id   = 0x0;

public:
    JUNCTION_PROXY_GET_NAME

    JUNCTION_PROXY_PASS_FNC_SIG {
        switch( rgh::txt_hash( line_ ) ) {
            case rgh::txt_hash( "install" ): {
                BridgE.install_dock( std::format( "{}-{}", JUNCTION_NAME, next_dock_id++ ), rgh::HVec< Topdon_TC001 >::make() );
                break; }

            default: return ERR_NO_RESOLVE;
        }
        return OK;
    }
};
JUNCTION_PROXY_INSTALL( Proxy )
JUNCTION_FOOTER
