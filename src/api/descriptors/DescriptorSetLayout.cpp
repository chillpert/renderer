#include "DescriptorSet.hpp"
#include "Components.hpp"

namespace rx
{
  DescriptorSetLayout::~DescriptorSetLayout( )
  {
    if ( m_layout )
      destroy( );
  }

  void DescriptorSetLayout::addBinding( const vk::DescriptorSetLayoutBinding& binding )
  {
    if ( m_layout )
      RX_ERROR( "Failed to add binding because the descriptor set layout was already initialized." );

    m_bindings.push_back( binding );
  }

  void DescriptorSetLayout::clearBindings( )
  {
    m_bindings.clear( );
  }

  void DescriptorSetLayout::init( )
  {
    vk::DescriptorSetLayoutCreateInfo createInfo( { },                                            // flags
                                                  static_cast< uint32_t >( m_bindings.size( ) ) , // bindingCount
                                                  m_bindings.data( ) );                           // pBindings
 
    m_layout = g_device.createDescriptorSetLayout( createInfo );
    if ( !m_layout )
      RX_ERROR( "Failed to create descriptor set layout." );
  }

  void DescriptorSetLayout::destroy( )
  {
    if ( m_layout )
    {
      g_device.destroyDescriptorSetLayout( m_layout );
      m_layout = nullptr;
    }

    m_bindings.clear( );
  }
}