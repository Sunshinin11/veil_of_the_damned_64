# Prompt Antigravity: subir o projeto para o GitHub

Cole o bloco abaixo no Antigravity. **Não peça testes nem build**: é só git.

---

Suba o projeto que está em `C:\Desenvolvimento\Nintendo 64` para o repositório **https://github.com/Sunshinin11/veil_of_the_damned_64**. Você já tem acesso ao meu GitHub.

**Regras:**
- **NÃO** rode build, testes, emulador nem scripts Python. **NÃO** apague, mova nem edite nenhum arquivo do projeto.
- O `.gitignore` já está pronto na raiz do projeto. Use-o como está: ele deixa de fora `build/`, `filesystem/`, `deps/`, a ROM, o APK, os logs do Ares e os backups.
- O `README.md` da raiz já está pronto, em português (BR) e com a logo do jogo (`Sprites/Menu/logo_veil_of_the_damned.png`). Ele aparece como página do repositório. **Não reescreva nem traduza.** O README antigo está guardado em `docs/README_ANTIGO.md`.
- Os sprites são muitos e grandes (cerca de 700 MB), mas nenhum arquivo passa de 20 MB. **Não** use Git LFS.

**Passos:**
1. No PowerShell, entre na pasta: `cd "C:\Desenvolvimento\Nintendo 64"`
2. Se ainda não existir repositório git aí (`git status` falha), rode `git init` e depois `git branch -M main`.
3. Remote:
   - Se não houver `origin`: `git remote add origin https://github.com/Sunshinin11/veil_of_the_damned_64.git`
   - Se já existir, ajuste com `git remote set-url origin https://github.com/Sunshinin11/veil_of_the_damned_64.git`
4. Para o push grande não cair, rode `git config http.postBuffer 524288000`.
5. Rode `git add -A` e depois `git commit -m "Veil of the Damned 64: versao 1.0 (Barbaro, Cacadora, Druida, 3 campanhas)"`.
6. Se o repositório no GitHub já tiver algum commit (README ou licença criados pelo site), rode antes `git pull origin main --allow-unrelated-histories --no-edit`. Em caso de conflito, mantenha a versão LOCAL.
7. Rode `git push -u origin main`. Se o push falhar por tamanho ou tempo, tente de novo uma vez. Se falhar de novo, me mostre o erro e pare.
8. Descrição e tópicos do repositório (só se o `gh` estiver instalado e logado):
   `gh repo edit Sunshinin11/veil_of_the_damned_64 --description "RPG de ação roguelike gótico para Nintendo 64 (libdragon + Tiny3D). Bárbaro, Caçadora e Druida; 3 campanhas procedurais." --add-topic nintendo-64 --add-topic n64 --add-topic homebrew --add-topic libdragon --add-topic roguelike --add-topic arpg`
   Se o `gh` não estiver disponível, pule este passo e o passo 9.
9. **Opcional, só se o `gh` (GitHub CLI) estiver instalado e logado:** crie uma release com a ROM e o APK anexados, sem colocá-los no repositório:
   `gh release create v1.0 "veil_of_the_damned.z64" "<caminho do APK que você gerou>" --title "Veil of the Damned 64 v1.0" --notes "Primeira versao completa."`
   Se o `gh` não estiver disponível, pule este passo e só me avise.

No final, me responda só com: o hash do commit, se o push deu certo e, se fez a release, o link dela.
