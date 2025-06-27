# Setup fzf
# ---------
export PATH="${PATH:+${PATH}:}${ENVDIR}/fzf/bin"

# Auto-completion
# ---------------
[[ $- == *i* ]] && source "${ENVDIR}/fzf/shell/completion.bash" 2> /dev/null

# Key bindings
# ------------
source "${ENVDIR}/fzf/shell/key-bindings.bash"
