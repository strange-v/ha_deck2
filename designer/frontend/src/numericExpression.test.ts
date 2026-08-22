import { describe, expect, it } from "vitest";
import { evaluateNumericExpression } from "./numericExpression";

describe("numeric expressions", () => {
  it("evaluates arithmetic", () => {
    expect(evaluateNumericExpression("10 + 2 * 3")).toBe(16);
    expect(evaluateNumericExpression("(10 + 2) / 3")).toBe(4);
    expect(evaluateNumericExpression("-8 + 3")).toBe(-5);
  });
  it("rejects invalid expressions", () => {
    expect(evaluateNumericExpression("10 / 0")).toBeNull();
    expect(evaluateNumericExpression("2 + nope")).toBeNull();
  });
});
