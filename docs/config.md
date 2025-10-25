# Configurazione AI-AutoShell

File di configurazione opzionale: `~/.ai-autoshellrc`

Formato:

- Ogni riga `chiave=valore`
- `#` inizia un commento
- Spazi non vengono trim-mati automaticamente (evita spazi attorno all'=')

Chiavi supportate:

| Chiave        | Descrizione                             | Default                 |
| ------------- | --------------------------------------- | ----------------------- |
| prompt_format | Template del prompt con placeholder     | `{user}@{host} {cwd}$ ` |
| color         | Abilita sequenze ANSI colore nel prompt | `true`                  |

Placeholder disponibili in `prompt_format`:

- `{user}` nome utente
- `{host}` hostname
- `{cwd}` ultima componente della directory corrente
- `{status}` exit status ultimo comando (`$?`)

Esempi:

```ini
# Prompt compatto con status e cwd tra parentesi
prompt_format={user}@{host} [{cwd}] (st={status})$
color=true
```

```ini
# Prompt senza colori
color=false
prompt_format={cwd}$
```

Colori:

- Colori attuali: user=verde, host=blu, cwd=giallo, status=rosso.
- Disabilitando `color` vengono rimossi i codici ANSI.

Note:

- Se il file non esiste vengono usati i default.
- Campi sconosciuti vengono ignorati.
- In futuro: alias, history, livelli di sicurezza LLM, plugin.

Errori comuni:

- Usare virgolette: non serve (`prompt_format="{user}"` → includerà le virgolette nel prompt).
- Inserire spazi dopo `=`: `prompt_format= {user}` produce spazio iniziale.

Per testare rapidamente:

```sh
cp ai-autoshellrc.example ~/.ai-autoshellrc
$EDITOR ~/.ai-autoshellrc
./build/ai-autoshell
```

Futuri parametri (roadmap):

- `history_size=1000`
- `danger_confirm=on`
- `planner_mode=suggest|auto`
