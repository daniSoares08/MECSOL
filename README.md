# MECSOL – Structural Mechanics Toolkit for TI-84 Plus CE

MECSOL is a **structural mechanics helper** written in C to run **directly on a TI-84 Plus CE**, using the **CEDev** toolchain.  
The goal is to speed up common calculations from Strength of Materials / Mechanics of Solids classes, directly on the calculator you already use in exams.

> ⚠️ **Important:** This code is designed **exclusively** for the **TI-84 Plus CE** and the **CEDev** toolchain.  
> It is **not** a generic C program for PCs or other calculators.

---

## ✨ Features

_Current features (and typical use cases):_

- 📐 **Centroid of composite areas**  
  - Handles sections built from multiple rectangles.  
  - Calculates global centroid coordinates based on local dimensions and offsets.

- 📊 **Area moment of inertia (second moment of area)**  
  - Computes the moment of inertia for composite sections using the parallel axis theorem.  
  - Useful for flexural stress and beam deflection formulas.

- 🧮 **Basic structural mechanics utilities**  
  - Designed to assist in Strength of Materials / Mechanics of Solids problems.  
  - Focus on step-by-step numeric output that is easy to follow on the calculator screen.

- 🎯 **Calculator-friendly UI**  
  - Menu navigation using TI-84 Plus CE keys.  
  - Values entered via the calculator keyboard, no PC required once the program is installed.

> You can adjust this section as new modules/features are implemented.

---

## 🧱 Project structure

A simplified view of the repository:

```text
MECSOL/
├─ src/           # All C source code for the calculator app (modules, menus, logic)
├─ Makefile       # Build rules for CEDev toolchain
└─ .gitignore     # Standard ignore rules (build artifacts, etc.)
````

Inside `src/` you will find the modules for centroid and moment of inertia calculations, as well as the main menu logic and drawing routines for the calculator screen.

---

## 🧰 Requirements

To build and run MECSOL you need:

* A **TI-84 Plus CE** calculator
* A working installation of **[CEDev](https://ce-programming.github.io/toolchain/)** (C toolchain for TI-84 Plus CE)
* **TI-Connect CE** (or compatible software) to transfer the compiled program (`.8xp` / app) to your calculator

---

## 🔨 Building with CEDev

1. **Install CEDev**

   Follow the official instructions from the CEDev documentation for your OS (Windows, Linux, etc.).
   Make sure the `make` command and CEDev tools are available in your terminal.

2. **Clone this repository**

   ```bash
   git clone https://github.com/daniSoares08/MECSOL.git
   cd MECSOL
   ```

3. **Build the project**

   Usually it is enough to run:

   ```bash
   make
   ```

   This will:

   * Compile the C source files in `src/`
   * Link them into a TI-84 Plus CE executable
   * Generate the output file ready to be sent to the calculator (e.g. `MECSOL.8xp` or similar, depending on the Makefile settings)

4. **Transfer to the calculator**

   * Open **TI-Connect CE**
   * Connect your **TI-84 Plus CE** via USB
   * Drag the generated program file to TI-Connect CE to send it to the calculator
   * On the calculator, find the program in the `PRGM`/`Apps` menu and run it

> If your build process uses a different command (e.g. `make gfx` before `make`), you can adjust this section accordingly.

---

## ⌨️ Usage

Once installed in the TI-84 Plus CE:

1. Open the **program/app MECSOL** from the calculator menu.
2. Use the **arrow keys** to navigate through the menus.
3. Press **[ENTER]** to select an option.
4. Press **[CLEAR]** to go back or exit a screen (depending on how the module is implemented).
5. Insert the geometrical/force data requested on screen:

   * Dimensions of each shape (e.g. `L`, `h`)
   * Position offsets (`x0`, `y0`)
   * Other variables depending on the module

The program will display the intermediate or final results according to your selections (centroid coordinates, area moment of inertia, stresses, etc.).

---

## 🛣 Roadmap / TODO

Some possible future improvements:

* Support for **more geometric shapes** (circles, triangles, etc.) in composite sections
* Modules for **bending stresses**, **normal stress**, and **shear** in beams with different load types
* Better **step-by-step visualisation** of formulas on the calculator screen
* Internationalization of on-calculator text (Portuguese/English)

Feel free to fork the project and experiment with your own modules.

---

## 🤝 Contributing

Contributions are welcome!
Possible ways to help:

* Reporting bugs and strange results in specific test cases
* Suggesting new structural/mechanics features
* Improving code structure and performance for the CEDev toolchain
* Enhancing UI/UX for the TI-84 Plus CE screen

If you open a pull request, please:

* Keep the code style consistent with the existing files
* Test the build with CEDev
* Test the program on a real TI-84 Plus CE or a reliable emulator when possible

---

## ⚖️ License

This project is released under a permissive non-commercial license.

You are free to:
- Use the software
- Modify the source code
- Redistribute modified or unmodified versions

As long as:
- You **do not use** this project or its derivatives for **commercial purposes**  
- You keep proper attribution to the original author: **Daniel Campos Soares**
- You include this license text in any redistributed version

For commercial usage or special permissions, please contact the author.

---

## 👤 Author

Developed by **Daniel Campos Soares**

* GitHub: [@daniSoares08](https://github.com/daniSoares08)

---

# MECSOL – Ferramentas de Mecânica dos Sólidos para TI-84 Plus CE (PT-BR)

MECSOL é um programa em C feito para rodar **diretamente na calculadora TI-84 Plus CE**, usando o toolchain **CEDev**.
O objetivo é agilizar contas de **Resistência dos Materiais / Mecânica dos Sólidos** na própria calculadora usada em prova.

> ⚠️ **Importante:** Este código foi feito **exclusivamente** para a **TI-84 Plus CE** e para o toolchain **CEDev**.
> Ele **não** é um programa genérico de C para PC ou para outras calculadoras e não foi testado nessas condições.

---

## ✨ Funcionalidades

*Funcionalidades:*

* 📐 **Cálculo de centroide de figuras compostas**

  * Trabalha com seções montadas a partir de vários retângulos.
  * Calcula as coordenadas do centroide global a partir das dimensões locais e deslocamentos.

* 📊 **Momento de inércia de área (segundo momento de área)**

  * Calcula o momento de inércia de seções compostas usando o teorema dos eixos paralelos.
  * Útil para fórmulas de tensão de flexão e flecha em vigas.

* 🧮 **Utilitários básicos de mecânica dos sólidos**

  * Pensado para ajudar em exercícios de Resistência dos Materiais / Mecânica dos Sólidos.
  * Foco em resultados numéricos claros e organizados para a tela da calculadora.

* 🎯 **Interface amigável na calculadora**

  * Navegação por menus usando as teclas da TI-84 Plus CE.
  * Inserção de valores direto na calculadora, sem depender do PC depois de instalado.

> Você pode adaptar esta seção conforme novos módulos/funcionalidades forem sendo implementados.

---

## 🧱 Estrutura do projeto

Visão simplificada do repositório:

```text
MECSOL/
├─ src/           # Código-fonte C do app da calculadora (módulos, menus, lógica)
├─ Makefile       # Regras de compilação para o toolchain CEDev
└─ .gitignore     # Regras de ignore (arquivos de build, etc.)
```

Dentro de `src/` estão os módulos de cálculo de centroide e momento de inércia, além da lógica principal de menus e desenho na tela da calculadora.

---

## 🧰 Requisitos

Para compilar e rodar o MECSOL você precisa de:

* Uma **TI-84 Plus CE**
* Uma instalação funcionando do **[CEDev](https://ce-programming.github.io/toolchain/)** (toolchain C para TI-84 Plus CE)
* **TI-Connect CE** (ou software compatível) para transferir o programa compilado (`.8xp` / app) para a calculadora

---

## 🔨 Compilando com CEDev

1. **Instale o CEDev**

   Siga a documentação oficial do CEDev para o seu sistema operacional (Windows, Linux, etc.).
   Garanta que o comando `make` e as ferramentas do CEDev estejam acessíveis no terminal.

2. **Clone este repositório**

   ```bash
   git clone https://github.com/daniSoares08/MECSOL.git
   cd MECSOL
   ```

3. **Compile o projeto**

   Normalmente basta rodar:

   ```bash
   make
   ```

   Isso irá:

   * Compilar os arquivos C dentro de `src/`
   * Linkar tudo em um executável da TI-84 Plus CE
   * Gerar o arquivo final pronto para ser enviado à calculadora (por exemplo `MECSOL.8xp`, dependendo da configuração do Makefile)

4. **Envie para a calculadora**

   * Abra o **TI-Connect CE**
   * Conecte a **TI-84 Plus CE** via USB
   * Arraste o arquivo gerado para o TI-Connect CE para enviá-lo
   * Na calculadora, procure o programa/app no menu `PRGM`/`Apps` e execute

> Se o seu fluxo de build exigir comandos extras (ex.: `make gfx` antes de `make`), ajuste esta seção conforme o seu Makefile.

---

## ⌨️ Uso

Depois de instalado na TI-84 Plus CE:

1. Abra o **programa/app MECSOL** no menu da calculadora.
2. Use as **setas** para navegar pelos menus.
3. Use **[ENTER]** para selecionar uma opção.
4. Use **[CLEAR]** para voltar ou sair de uma tela (dependendo de como o módulo está implementado).
5. Preencha os dados geométricos/forças pedidos na tela:

   * Dimensões de cada forma (por exemplo `L`, `h`)
   * Deslocamentos (`x0`, `y0`)
   * Outras variáveis, dependendo do módulo

O programa exibirá os resultados intermediários ou finais conforme suas escolhas (coordenadas do centroide, momento de inércia, tensões, etc.).

---

## 🛣 Roadmap / Próximos passos

Possíveis melhorias futuras:

* Suporte a **mais formas geométricas** (círculos, triângulos etc.) em áreas compostas
* Módulos para **tensões de flexão**, **tensão normal** e **corte** em vigas com diferentes tipos de carregamento
* Melhor **visualização passo a passo** das fórmulas na tela da calculadora
* Textos da interface em **Português/Inglês** na própria calculadora

Sinta-se à vontade para fazer fork do projeto e testar suas próprias ideias.

---

## 🤝 Contribuindo

Contribuições são bem-vindas!
Você pode ajudar:

* Reportando bugs e resultados estranhos em casos de teste específicos
* Sugerindo novas funcionalidades de mecânica/estrutura
* Melhorando a organização do código e desempenho voltado para o CEDev
* Deixando a interface mais intuitiva para a tela da TI-84 Plus CE

Ao abrir um pull request:

* Mantenha o estilo de código consistente com o que já existe
* Teste a compilação com o CEDev
* Teste o programa em uma TI-84 Plus CE real ou em um emulador confiável, se possível

---

## ⚖️ Licença

Este projeto é distribuído sob uma licença permissiva de uso não comercial.

Você tem permissão para:
- Usar o software
- Modificar o código-fonte
- Redistribuir versões modificadas ou não modificadas

Desde que:
- Você **não utilize** este projeto ou derivados para **fins comerciais**
- Mantenha o devido crédito ao autor original: **Daniel Campos Soares**
- Inclua este texto de licença em qualquer versão redistribuída

Para uso comercial ou permissões especiais, entre em contato com o autor.

---

## 👤 Autor

Desenvolvido por **Daniel Campos Soares**

* GitHub: [@daniSoares08](https://github.com/daniSoares08)

