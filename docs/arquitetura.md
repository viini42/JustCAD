# JustCAD — Rascunho de arquitetura (v0)

> Status: v1, decisões principais fechadas em 2026-11-03 (§11). Itens com **[premissa]** ainda podem mudar.

## 1. Objetivo

Modelador 2D que desenha pontos, linhas, poligonais, curvas Bézier e regiões fechadas (retângulo, polígono regular, círculo, elipse, região delimitada por primitivas). Internamente, tudo é representado por uma estrutura topológica **half-edge** manipulada exclusivamente por **operadores de Euler**, em camadas com dependência estritamente descendente.

## 2. Visão geral das camadas

```
 ┌──────────────────────────────────────────────┐
 │ 5. App / UI (janela, input, render, comandos) │
 ├──────────────────────────────────────────────┤
 │ 4. Modelo geométrico (API pública: addLine,   │
 │    addCircle, split, delete, query, undo)     │
 ├──────────────────────────────────────────────┤
 │ 3. Operadores de Euler de alto nível          │
 │    (makeEdgeBetween, insertVertexOnEdge,      │
 │     closeRegion, removeEdge, ...)             │
 ├──────────────────────────────────────────────┤
 │ 2. Operadores de Euler de baixo nível         │
 │    (MVFS/KVFS, MEV/KEV, MEF/KEF, KEMR/MEKR)   │
 ├──────────────────────────────────────────────┤
 │ 1. Estrutura topológica half-edge (dados)     │
 └──────────────────────────────────────────────┘
        ▲ lateral: Geometria (Curve, Point2, BBox) — sem dependência de topologia
```

Regra de acoplamento: cada camada só conhece a imediatamente inferior (e a biblioteca de geometria). A topologia **não sabe** o que é uma reta ou uma Bézier; ela guarda apenas um handle opaco para a geometria da aresta.

## 3. Camada 1 — Estrutura half-edge

Entidades (armazenadas em pools/arrays com IDs inteiros estáveis, não ponteiros crus — facilita undo, serialização e validação):

| Entidade | Campos principais |
|---|---|
| `Vertex` | `HalfEdgeId outgoing`, `GeomPointId point` |
| `HalfEdge` | `VertexId origin`, `HalfEdgeId twin, next, prev`, `LoopId loop`, `EdgeId edge` |
| `Edge` | `HalfEdgeId he[2]`, `CurveId curve`, sentido da curva relativo a `he[0]` |
| `Loop` | `HalfEdgeId first`, `FaceId face`, `bool outer` |
| `Face` | `LoopId outer`, `vector<LoopId> inner` (buracos), atributos |
| `Shell`/`Model` | conjuntos de faces/vértices/arestas; contém a face "infinita" |

Decisões:
- **Face ilimitada explícita** ✅ *(decidido em 2026-11-03)*: o plano é uma face; toda primitiva vive dentro dela. Isso torna linhas soltas, pontos isolados e regiões tratáveis da mesma forma.
- **Loops com buracos** (`inner loops`) ✅ *(decidido em 2026-11-03)*. Nota: além de furos, é o mecanismo que guarda cada desenho desconexo dentro da face ilimitada, então é usado o tempo todo internamente. Para suportar componentes desconexos e regiões furadas (necessário para KEMR/MEKR).
- **Aresta "cabelo"** ✅ *(decidido em 2026-11-03)* (half-edges gêmeas no mesmo loop) para linhas abertas e poligonais que não fecham região.
- Invariante de Euler-Poincaré 2D verificável por um `validate()` em modo debug ✅ *(decidido em 2026-11-03)*. Com a face ilimitada contada em `F`, a fórmula fica `V − E + F = 1 + H`, onde `H` é o número de loops internos (igual ao número de componentes conexos `C`, já que cada componente vive como exatamente um loop interno da face que o contém; vértice isolado conta como loop). Exemplos: vértice isolado 1−0+1 = 2 (H=1); triângulo 3−3+2 = 2 (H=1); duas linhas soltas 4−2+1 = 3 (H=2); quadrado dentro de quadrado, sem tocar, 8−8+3 = 3 (H=2). O `validate()` também checa a consistência local (`twin(twin(h)) = h`, `next(prev(h)) = h`, todos os half-edges de um loop apontam para ele).

## 4. Camada 2 — Operadores de Euler de baixo nível

Conjunto mínimo e completo (com inversos), no estilo Mäntylä:

| Operador | Efeito | Inverso |
|---|---|---|
| `MVFS` | cria vértice + face + shell (primeiro ponto) | `KVFS` |
| `MEV` | cria aresta e vértice a partir de um vértice existente | `KEV` |
| `MEF` | cria aresta entre dois vértices do mesmo loop, dividindo a face | `KEF` |
| `KEMR` | remove aresta criando anel (loop interno) | `MEKR` |
| `MVR`/`KVR` (opcional) | vértice isolado como anel dentro de face | — |

Regras:
- Cada operador valida pré-condições, altera apenas ponteiros/IDs e **retorna um registro do inverso** → base natural para undo/redo.
- Nenhuma geometria é calculada aqui; os operadores recebem IDs de geometria já prontos.

## 5. Camada 3 — Operadores de Euler de alto nível

Composições dos de baixo nível com semântica de modelagem, ainda puramente topológicas (mas podendo consultar geometria para decidir em qual loop inserir):

- `addIsolatedVertex(p)`
- `addEdge(v1, v2, curve)` — escolhe MEV, MEF ou MEKR conforme v1/v2 estejam conectados, no mesmo loop, em loops distintos.
- `splitEdge(e, t)` — insere vértice numa aresta (divide a curva em duas).
- `mergeEdges(v)` — remove vértice de grau 2 (inverso de `splitEdge`).
- `removeEdge(e)` — escolhe KEV, KEF ou KEMR.
- `removeVertex(v)`.
- Transações: agrupar vários operadores numa única entrada de undo.

## 6. Camada 4 — Modelo geométrico (API pública)

Funções que o resto do app usa; traduz primitivas em topologia:

| Primitiva | Construção topológica |
|---|---|
| Ponto | vértice isolado |
| Linha | 2 vértices + 1 aresta (`LineSegment`) |
| Poligonal aberta | n vértices + n−1 arestas (ou 1 aresta `Polyline` — ver §7) |
| Bézier | 2 vértices + 1 aresta (`Bezier`) |
| Retângulo / polígono regular | n vértices + n arestas fechando uma face (`MEF` no fim) |
| Círculo / elipse | 1 vértice + 1 aresta fechada (laço) `Arc/Ellipse`, ou 2 vértices + 2 arcos **[premissa]** |
| Região por primitivas | **interseção planar**: ao inserir uma aresta, calcula interseções com as existentes, divide arestas (`splitEdge`) e insere os pedaços; faces surgem automaticamente via `MEF` |

Responsabilidades extras desta camada: interseção curva×curva, snapping/tolerância, consultas (ponto em face, aresta mais próxima, bbox), índice espacial (inicialmente força bruta, depois grid/quadtree).

## 7. Abstração genérica de aresta (geometria)

Interface independente da topologia:

```cpp
class Curve {
public:
    virtual Point2 eval(double t) const = 0;         // t ∈ [0,1]
    virtual Vec2   tangent(double t) const = 0;
    virtual BBox   bbox() const = 0;
    virtual double length() const = 0;
    virtual double closestParam(Point2 p) const = 0;
    virtual std::pair<CurvePtr,CurvePtr> split(double t) const = 0;
    virtual CurvePtr reversed() const = 0;
    virtual void   tessellate(double tol, std::vector<Point2>& out) const = 0;
    virtual CurveKind kind() const = 0;
};
// LineSegment, Polyline, Bezier (grau arbitrário), CircularArc, EllipticArc
```

**Bézier de grau arbitrário** ✅ *(decidido)*: `Bezier { std::vector<Point2> ctrl; }` com grau `n = ctrl.size() − 1`. Avaliação e `split(t)` por de Casteljau; derivada é uma Bézier de grau `n−1`; `bbox` pelo fecho convexo dos pontos de controle (conservadora) e refinada pelas raízes da derivada. Interseção Bézier×qualquer por subdivisão recursiva com descarte por bbox (ou *Bézier clipping*), terminando em refinamento de Newton. Elevação de grau disponível para operações que precisem igualar graus.

- Interseções implementadas por despacho duplo ou tabela `(kind, kind)` → funções especializadas (linha×linha analítica, demais via subdivisão de Bézier/tessellação + refinamento de Newton).
- Alternativa a herança virtual: `std::variant<LineSegment, Polyline, Bezier, Arc, ...>` + `std::visit` (sem alocação, mais fácil de serializar). **Recomendo `variant`**; decisão aberta.
- **Poligonal**: recomendo representá-la como *n arestas de reta* (cada vértice interno é um vértice topológico), agrupadas por um atributo "entidade" para seleção/edição. Mantém a topologia honesta (interseções em vértices internos funcionam sem casos especiais). `Polyline` como Curve fica disponível caso se prefira uma aresta só.

### 7.1 Kernel numérico (troca double ↔ aritmética exata)

✅ *(decidido)*: `double` com tolerância global agora, mas atrás de uma abstração que permita trocar por aritmética exata depois. Proposta, inspirada nos *kernels* do CGAL:

```cpp
template <class K>
concept Kernel = requires {
    typename K::Scalar;                      // double, ou mpq_class / racional exato
    typename K::Point2;  typename K::Vec2;
} && requires (typename K::Point2 a, typename K::Point2 b, typename K::Point2 c) {
    { K::orientation(a, b, c) } -> std::same_as<Sign>;   // predicado: esquerda/direita/colinear
    { K::equal(a, b) }          -> std::same_as<bool>;   // usa tolerância no kernel double
    { K::compareX(a, b) }       -> std::same_as<Sign>;
};

struct DoubleKernel { using Scalar = double; static inline double eps = 1e-9; ... };
// futuro: ExactKernel (racionais), FilteredKernel (double com fallback exato, estilo Shewchuk)
```

Regras para a troca ser simples de verdade:
- **Toda decisão topológica passa por predicados do kernel** (`orientation`, `equal`, `compare`, ponto-em-segmento). Nenhum `if (abs(x) < 1e-9)` espalhado pelo código.
- Construções (ponto de interseção, ponto em `t`) ficam separadas dos predicados, porque num kernel exato elas podem ser caras ou impossíveis (interseções de Bézier não são racionais); para curvas, o kernel exato pode cair em intervalos/filtros.
- `geom/`, `euler/high/` e `model/` são parametrizados por `K` (template). `topo/` e `euler/low/` não dependem do kernel (só IDs).
- Tolerância global configurável no kernel double, com testes que rodam a mesma bateria sob diferentes valores de `eps`.

## 8. Entidades de usuário vs. topologia

Camada fina acima do modelo: `Entity { id, kind (Rect, Circle, Polyline...), params, vector<EdgeId> }`. Permite editar "o retângulo" como objeto mesmo após a topologia ser dividida por interseções. Cada `Edge` guarda o(s) `EntityId` de origem.

## 9. Estrutura de código proposta (C++23 + CMake)

```
JustCAD/
  CMakeLists.txt
  src/
    geom/      Point2, Vec2, BBox, Curve (variant), intersect, tolerance
    topo/      HalfEdge DS, ids/pools, validate()
    euler/     low/ (MVFS, MEV, MEF, KEMR + inversos)  high/ (addEdge, splitEdge...)
    model/     GeometricModel (API pública), entities, undo/redo
    app/       janela, render, ferramentas de desenho   (depende só de model/)
  tests/       um alvo por camada (Catch2 ou GoogleTest)
```

Cada pasta é uma biblioteca CMake separada com dependências explícitas — o próprio build impede violações de camada.

## 10. Roadmap sugerido

1. **M0** — esqueleto CMake, CI, `geom/` básico (Point2, LineSegment) com testes.
2. **M1** — half-edge + operadores baixo nível + `validate()`; testes de invariantes e de "operador seguido do inverso = identidade".
3. **M2** — operadores alto nível + undo/redo por transação.
4. **M3** — modelo geométrico com linhas/poligonais/retângulos/polígonos regulares (só retas) + interseção reta×reta → regiões automáticas.
5. **M4** — UI mínima: desenhar, selecionar, apagar, visualizar faces preenchidas.
6. **M5** — Bézier, arcos, círculos, elipses + interseções curvas.
7. **M6** — persistência (formato JSON próprio), import/export (SVG/DXF), consultas avançadas.

## 11. Decisões (2026-11-03)

1. Linguagem/stack: **C++23 + CMake**.
2. UI/renderização: **SDL3 + Dear ImGui**.
3. Regiões por primitivas: **detecção automática** pelas interseções ao desenhar.
4. Bézier: **grau arbitrário** (§7).
5. Precisão: **`double` com tolerância global**, atrás de um kernel trocável por aritmética exata (§7.1).
