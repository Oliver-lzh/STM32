# =================================================================================================
# File:     mk_extensions.mk
# -------------------------------------------------------------------------------------------------
# Brief:    Additional functions or targets defined to enhance the makefile system 
#           with missing functionality.
#
# =================================================================================================

current_dir=$(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
rwildcard=$(wildcard $1/$2) $(foreach d,$(wildcard $1/*/.),$(call rwildcard,$(d:/.=),$2))
lowercase=$(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))
_pos = $(if $(findstring $1,$2),$(call _pos,$1,$(wordlist 2,$(words $2),$2),x $3),$3)
pos = $(words $(call _pos,$1,$2))
