#ifndef __internals_common_h__
#define __internals_common_h__

/* CPU definitions */
#if defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
  #define CONFIG_CORTEX_A9      /* all DIGIC 7 and 8 models */
#elif defined(CONFIG_DIGIC_VI)
  #define CONFIG_CORTEX_R4      /* all DIGIC 6 models */
#else
  #define CONFIG_ARM946EOS      /* DIGIC 5 and earlier; it is slightly different from ARM946E-S */
#endif

/* DIGIC families */
#if defined(CONFIG_VXWORKS)
  #define CONFIG_DIGIC_23       /* synonym, unless we'll ever support DIGIC 1 */
  #define CONFIG_DIGIC_2345     /* sometimes, these are also similar */
#endif

#if defined(CONFIG_DIGIC_V) || defined(CONFIG_DIGIC_IV)
  #define CONFIG_DIGIC_45       /* these are very similar */
  #define CONFIG_DIGIC_2345     /* sometimes, these are too */
#endif

#if defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
  #define CONFIG_DIGIC_78       /* these are mostly identical */
#endif

#if defined(CONFIG_DIGIC_VI) || defined(CONFIG_DIGIC_VII) || defined(CONFIG_DIGIC_VIII)
  #define CONFIG_DIGIC_678      /* these are also very similar, more often than not */
#endif

/* Common internals
 * Careful - they cannot be overridden from internals.h!
 */
#ifdef CONFIG_DIGIC_78
  #define CONFIG_DUAL_CORE // distinct from Dual Digic, which is two CPUs on the same board,
                           // this is real dual core, shared memory, L2 caches etc.
#endif

#ifdef CONFIG_DIGIC_678
  /* All recent models use new-style DryOS task hooks */
  #define CONFIG_NEW_DRYOS_TASK_HOOKS
#endif

#if defined(CONFIG_DIGIC_V) || defined(CONFIG_DIGIC_678)
  /* Assuming all recent models use REC.709 */
  /* TODO: need a trained eye to check :) */
  #define CONFIG_REC709
#endif

#endif /* __internals_common_h__ */
