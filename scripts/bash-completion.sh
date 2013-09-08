_instantsend_client() {
	local cur prev opts CFG i
	COMPREPLY=()
	cur="${COMP_WORDS[COMP_CWORD]}"
	prev="${COMP_WORDS[COMP_CWORD-1]}"
	opts="-c -t -f"

	CFG=$HOME/.instantsend/client.cfg
	for i in $(seq 1 $(expr $COMP_CWORD - 2));
	do
		if [[ "${COMP_WORDS[i]}" == "-c" ]];
		then
			CFG="${COMP_WORDS[i+1]}"
		fi
	done

	if [[ "${prev}" == "-t" ]];
	then
		COMPREPLY=( $(compgen -W "$(instsend-config -c "$CFG" -T)" -- ${cur}) )
	else
		if [[ "${prev}" == "-c" ]] || [[ "${prev}" == "-f" ]];
		then
			COMPREPLY=( $(compgen -W "$(ls -A)" -- ${cur}) )
		else
			COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
		fi
	fi
}
complete -F _instantsend_client isend
