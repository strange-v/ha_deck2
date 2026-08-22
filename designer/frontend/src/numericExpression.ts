export function evaluateNumericExpression(source: string): number | null {
  let index = 0;
  const skip = () => { while (/\s/.test(source[index] ?? "")) index += 1; };
  const expression = (): number | null => {
    let value = term();
    if (value === null) return null;
    while (true) {
      skip(); const operator = source[index];
      if (operator !== "+" && operator !== "-") break;
      index += 1; const right = term();
      if (right === null) return null;
      value = operator === "+" ? value + right : value - right;
    }
    return value;
  };
  const term = (): number | null => {
    let value = factor();
    if (value === null) return null;
    while (true) {
      skip(); const operator = source[index];
      if (operator !== "*" && operator !== "/") break;
      index += 1; const right = factor();
      if (right === null || (operator === "/" && right === 0)) return null;
      value = operator === "*" ? value * right : value / right;
    }
    return value;
  };
  const factor = (): number | null => {
    skip();
    if (source[index] === "+" || source[index] === "-") {
      const sign = source[index++] === "-" ? -1 : 1;
      const value = factor(); return value === null ? null : sign * value;
    }
    if (source[index] === "(") {
      index += 1; const value = expression(); skip();
      if (value === null || source[index] !== ")") return null;
      index += 1; return value;
    }
    const match = /^(?:\d+(?:\.\d*)?|\.\d+)/.exec(source.slice(index));
    if (!match) return null;
    index += match[0].length; return Number(match[0]);
  };
  const value = expression(); skip();
  return value !== null && index === source.length && Number.isFinite(value) ? value : null;
}
