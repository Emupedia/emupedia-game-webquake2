#ifndef HUMBLENET_H
#define HUMBLENET_H


#include <stddef.h>
#include <stdint.h>

#ifdef HUMBLENET_STATIC
	#define HUMBLENET_DLL_EXPORT
#else
	#ifdef WIN32
		#ifdef HUMBLENET_DLL_BUILD
			#define HUMBLENET_DLL_EXPORT __declspec(dllexport)
		#else
			#define HUMBLENET_DLL_EXPORT __declspec(dllimport)
		#endif
	#else // GCC
		#if defined(__GNUC__) && __GNUC__ >= 4
		# define HUMBLENET_DLL_EXPORT __attribute__ ((visibility("default")))
		#else
		# define HUMBLENET_DLL_EXPORT
		#endif
	#endif
#endif

#if defined(WIN32)
	#define HUMBLENET_CALL __cdecl
#elif defined(__GNUC__)
	#if defined(__LP64__)
		#define HUMBLENET_CALL
	#else
		#define HUMBLENET_CALL __attribute__((cdecl))
	#endif
#else
	#define HUMBLENET_CALL
#endif

#define HUMBLENET_API HUMBLENET_DLL_EXPORT


#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t ha_bool;
typedef uint32_t PeerId;


/*
 * Initialize the core humblenet library
 */
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_init();

/*
 * Shutdown the entire humblenet library
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_shutdown();

/*
 * If using threads, you must lock before any calls to the humblent_apis.
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_lock();

/*
 * If using threads, you must unlock when done with any calls to the humblent_apis.
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_unlock();

/*
 * Allow network polling to occur. This is already thread safe and must NOT be within a lock/unlock block
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_poll(int ms);

/*
 * Get error string
 * Return value will stay valid until next call to humblenet_set_error
 * or humblenet_clear_error() is called
 */
HUMBLENET_API const char * HUMBLENET_CALL humblenet_get_error();

/*
 * Set error string
 * Parameter is copied to private storage
 * Must not be NULL
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_set_error(const char *error);


/*
 * Clear error string
 */
HUMBLENET_API void HUMBLENET_CALL humblenet_clear_error();

#ifdef __cplusplus
}
#endif


#endif /*HUMBLENET_H */
