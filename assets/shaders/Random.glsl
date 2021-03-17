#define M_PI 3.141592

// @Nvidia vk_ray_tracing_KHR tutorial
// Generate a random unsigned int from two unsigned int values, using 16 pairs
// of rounds of the Tiny Encryption Algorithm. See Zafar, Olano, and Curtis,
// "GPU Random Numbers via the Tiny Encryption Algorithm"
uint tea( uint val0, uint val1 )
{
  uint v0 = val0;
  uint v1 = val1;
  uint s0 = 0;

  for ( uint n = 0; n < 16; n++ )
  {
    s0 += 0x9e3779b9;
    v0 += ( ( v1 << 4 ) + 0xa341316c ) ^ ( v1 + s0 ) ^ ( ( v1 >> 5 ) + 0xc8013ea4 );
    v1 += ( ( v0 << 4 ) + 0xad90777d ) ^ ( v0 + s0 ) ^ ( ( v0 >> 5 ) + 0x7e95761e );
  }

  return v0;
}

// @Nvidia vk_ray_tracing_KHR tutorial
// Generate a random unsigned int in [0, 2^24) given the previous RNG state
// using the Numerical Recipes linear congruential generator
uint lcg( inout uint prev )
{
  uint LCG_A = 1664525u;
  uint LCG_C = 1013904223u;
  prev       = ( LCG_A * prev + LCG_C );
  return prev & 0x00FFFFFF;
}

// @Nvidia vk_ray_tracing_KHR tutorial
// Generate a random float in [0, 1) given the previous RNG state
float rnd( inout uint prev )
{
  return ( float( lcg( prev ) ) / float( 0x01000000 ) );
}

// From Nvidia's vk_mini_path_tracer and ultimately from Peter Shirley's "Ray Tracing in one Weekend"
// Randomly sampling in hemisphere
vec3 samplingHemisphere( inout uint seed, in vec3 normal )
{
  // reduce fireflies
  // but results in loss of energy
  float u0 = clamp( rnd( seed ), 0.0, 3.0 );
  float u1 = clamp( rnd( seed ), 0.0, 3.0 );

  const float theta = M_PI * 2 * u0;  // Random in [0, 2pi]
  const float u     = 2.0 * u1 - 1.0; // Random in [-1, 1]
  const float r     = sqrt( 1.0 - u * u );
  vec3 rayDirection = normal + vec3( r * cos( theta ), r * sin( theta ), u );

  return normalize( rayDirection );
}

// @https://www.iue.tuwien.ac.at/phd/ertl/node100.html
vec3 samplingUnitSphere2( inout uint seed )
{
  float a;
  float a1;
  float a2;

  do
  {
    a1 = 2.0 * rnd( seed ) - 1;
    a2 = 2.0 * rnd( seed ) - 1;
    a  = a1 * a1 + a2 + a2;
  } while ( a < 1 );

  vec3 dir;
  dir.x = 1.0 - 2.0 * a;
  a     = 2 * sqrt( 1.0 - a );
  dir.y = a1 * a;
  dir.z = a2 * a;

  return normalize( dir );
}

// @https://www.iue.tuwien.ac.at/phd/ertl/node100.html
// Cosine distributed
// not working
vec3 samplingCone( inout uint seed, vec3 v )
{
  vec3 dir;
  float a;

  do
  {
    dir = samplingUnitSphere2( seed );
    dir += v;
    a = sqrt( dir.x * dir.x + dir.y * dir.y + dir.z * dir.z );
  } while ( a != 0.0 );

  dir = dir / a;
  return normalize( dir );
}

// Randomly sampling in hemisphere ( Peter Shirley's "Ray Tracing in one Weekend" )
vec3 samplingUnitSphere( inout uint seed )
{
  // @todo consider adding clamping here as well

  vec3 pos = vec3( 0.0 );
  do {
    pos = vec3( rnd( seed ), rnd( seed ), rnd( seed ) ) * 2.0 - 1.0;
  } while ( dot( pos, pos ) >= 1.0 );

  return pos;
}

// Randomly sampling in hemisphere ( Peter Shirley's "Ray Tracing in one Weekend" )
float Schlick( const float cosine, const float ni )
{
  float r0 = ( 1 - ni ) / ( 1 + ni );
  r0 *= r0;
  return r0 + ( 1 - r0 ) * pow( 1 - cosine, 5 );
}

bool refract2( inout vec3 v, inout vec3 n, float niOverNt, inout vec3 refracted )
{
  vec3 uv            = normalize( v );
  float dt           = dot( uv, n );
  float discriminant = 1.0 - niOverNt * niOverNt * ( 1 - dt * dt );
  if ( discriminant > 0 )
  {
    refracted = niOverNt * ( uv - n * dt ) - n * sqrt( discriminant );
    return true;
  }
  else
  {
    return false;
  }
}
