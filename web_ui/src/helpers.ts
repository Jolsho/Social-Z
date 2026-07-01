export function el<K extends keyof HTMLElementTagNameMap>(
  tag: K,
  props: Partial<HTMLElementTagNameMap[K]> = {},
  children: (Node | string)[] = []
) {
  const element = document.createElement(tag)
  Object.assign(element, props)
  element.append(...children)
  return element
}
