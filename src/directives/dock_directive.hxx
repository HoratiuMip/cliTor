#pragma once /*
# FILE: directives/dock_directive.hxx
# AUTHOR(s): Vatca "Mipsan" Tudor-Horatiu
#   Copyright (c) [2026]. All rights reserved.
#   Licensed under the MIT License. See the LICENSE file in the project root for full license information.
#
# DETAILS: Dock related stuff.
*/
#include "../descriptor.hxx"

#include <rgh/gep/dispenser.hpp>
#include <rgh/osp/immersive.hpp>

/**
 * @brief A dock must use these macros to print to its logger.
 */
#define JUNCTION_DOCK_LOGI( ... ) _dock_logger->info( __VA_ARGS__ )
#define JUNCTION_DOCK_LOGW( ... ) _dock_logger->warn( __VA_ARGS__ )
#define JUNCTION_DOCK_LOGE( ... ) _dock_logger->error( __VA_ARGS__ )

#define JUNCTION_PROXY_PASS_BASIC_DOCK_INSTALL( dock_t_ ) \
    case rgh::txt_hash( "install" ): { \
        static int _next_dock_id = 0; \
        BridgE.install_dock( std::format( "{}-{}", JUNCTION_NAME, _next_dock_id++ ), rgh::HVec< dock_t_ >::make() ); \
        break; }

#define JUNCTION_DOCK_STOP_OR_BRIDGE_STOP \
    (this->dock_stop_signaled() or BridgE.status() != OK)

#define JUNCTION_DOCK_WITH_BRIDGE_IMM_AND_UIX_PACK( pack_t_ ) \
    auto imm = BridgE.uix_imm_strong(); auto _raw_uix_pack = this->_uix_pack; auto uix_pack = reinterpret_cast< pack_t_* >( _raw_uix_pack.get() ); \
    ASSERT_AND( imm and uix_pack )
    
#define JUNCTION_DOCK_UIX_REINTR_PACK( pack_t_ ) auto* pack = reinterpret_cast< pack_t_* >( args_.pack );    

CLITOR_NAMESPACE {

class Dock {
public: friend class Bridge;
        friend class Dock_Directive;

protected:
//# Logger used by the dock.
    std::shared_ptr< spdlog::logger >   _dock_logger   = nullptr;
//# The id of the dock. Stored in the dock instance itself as well.
    std::string                         _dock_id       = {};

public:
/**
 * @brief Returns the id of the dock.
 * @note Still in doubt whether to return a string or a string_view, since dock ids
 *         will most of the time fit in SSOs. However either way the memory of the id
 *         itself still needs dereferencing.
 */
    std::string dock_id( void ) const { return _dock_id; }
/**
 * @brief Returns the id without the flagging information.
 */
    const char* dock_id_c_str( void ) const { 
        const int forced_ord_id_offset = _dock_id.starts_with( '/' ) ? 2 : 0;
        return &_dock_id[ forced_ord_id_offset ];
    }
/**
 * @brief This function informs the bridge that this dock may never be uninstalled.
 * @note The macro JUNCTION_DOCK_IS_PERSISTENT is provided to override this function.
 */
    virtual constexpr bool dock_is_persistent() const { return false; }
#define JUNCTION_DOCK_IS_PERSISTENT \
    virtual constexpr bool dock_is_persistent() const override { return true; }
/**
 * @brief Pass a command line to the dock.
 * @note The macro JUNCTION_DOCK_PASS_FNC_SIG is provided to replace the function signature.
 */
    virtual status_t dock_pass( std::string line_ ) { return ERR_NOT_IMPL; }
#define JUNCTION_DOCK_PASS_FNC_SIG \
    virtual status_t dock_pass( std::string line_ ) override 

#pragma region UIX
public:
/**
 * @brief Structure created and used during active UIX.
 */
    class UIX_pack {
        public: virtual ~UIX_pack() = default;
    };
/**
 * @brief Structure passed as argument for every frame of the UIX.
 */
    struct dock_uix_frame_args_t : rgh::Immersive::frame_cb_args_t {
    /**
     * @brief The UIX pack returned by the UIX begin function, if any.
     */
        UIX_pack*   pack   = nullptr;
    };

public:
/**
 * @brief Function called by the bridge once every time the UIX is started.
 * @note The macro JUNCTION_DOCK_UIX_BEGIN_FNC_SIG is provided to replace the function signature.
 */
    virtual std::shared_ptr< UIX_pack > dock_uix_begin() { return nullptr; }
#define JUNCTION_DOCK_UIX_BEGIN_FNC_SIG \
    virtual std::shared_ptr< ::Dock::UIX_pack > dock_uix_begin() override
/**
 * @brief Function called by the bridge every frame of the UIX.
 * @warning This function is not called when another dock is focused or during platform specific
 *            cases, i.e. the desktop environment locked the screen. Therefore do not expect this 
 *            function to be called consistently.
 * @note The macro JUNCTION_DOCK_UIX_FRAME_FNC_SIG is provided to replace the function signature.
 */
    virtual status_t dock_uix_frame( const dock_uix_frame_args_t& C ) { return ERR_NOT_IMPL; }
#define JUNCTION_DOCK_UIX_FRAME_FNC_SIG \
    virtual status_t dock_uix_frame( const dock_uix_frame_args_t& C ) override
/**
 * @brief Function called by the bridge once every time the UIX is stopped.
 * @note The macro JUNCTION_DOCK_UIX_END_FNC_SIG is provided to replace the function signature.
 */
    virtual void  dock_uix_end() { return; }
#define JUNCTION_DOCK_UIX_END_FNC_SIG \
    virtual void dock_uix_end() override

protected:
    std::shared_ptr< UIX_pack >   _uix_pack   = nullptr;
#pragma endregion UIX
};

class Dock_Directive {
public: _DIRECTIVE_FRIENDS

protected:
    struct _dock_key_t {
        std::string   id   = {};

        struct less {
            using is_transparent = void;

            inline bool operator () ( const _dock_key_t& lhs_, const _dock_key_t& rhs_ ) const { return lhs_.id < rhs_.id; }
            inline bool operator () ( const _dock_key_t& lhs_, std::string rhs_ ) const { return lhs_.id < rhs_; }
            inline bool operator () ( const _dock_key_t& lhs_, std::string_view rhs_ ) const { return lhs_.id < rhs_; }
            inline bool operator () ( const _dock_key_t& lhs_, const char* rhs_ ) const { return lhs_.id < rhs_; }
        };
    };
    struct _dock_entry_t {
        rgh::HVec< Dock >   ref   = nullptr;

        inline auto operator->() const { return ref.operator->(); }
    };

protected:
    rgh::Dispenser< std::map< _dock_key_t, _dock_entry_t, _dock_key_t::less > >   _dock_tbl   = { rgh::DispenserMode_Lock };

protected:
    void _dock_entry_set_id(
        IN   _dock_entry_t&   dken_,
        IN   std::string      id_
    );

    void _dock_entry_make_logger(
        IN   _dock_entry_t&   dken_
    );

    void _dock_entry_load_uix(
        IN   _dock_entry_t&   dken_
    );

    void _dock_drop_hooks(
        IN   _dock_entry_t&   dken_
    );

public:
    ret_t install_dock(
        IN   std::string           id_,
        IN   rgh::HVec< Dock >&&   dock_
    );

    ret_t uninstall_dock(
        IN   std::string_view   id_
    );

    rgh::HVec< Dock > dock_by_id( 
        IN   std::string_view   id_,
        IN   int                tol_ = 0
    );
};

}//# CLITOR_NAMESPACE