import { useEffect, useState } from "react";
import { evaluateNumericExpression } from "./numericExpression";

export default function NumericExpressionInput({ value, disabled, min, onCommit }: {
  value: number | string; disabled?: boolean; min?: number; onCommit: (value: number) => void;
}) {
  const [draft, setDraft] = useState(String(value));
  const [invalid, setInvalid] = useState(false);
  useEffect(() => { setDraft(String(value)); setInvalid(false); }, [value]);
  const commit = () => {
    const evaluated = evaluateNumericExpression(draft);
    if (evaluated === null) { setInvalid(true); return; }
    const result = Math.max(min ?? -Infinity, Math.round(evaluated));
    setDraft(String(result)); setInvalid(false);
    if (result !== value) onCommit(result);
  };
  return <input type="text" inputMode="decimal" className={invalid ? "invalid" : undefined}
    value={draft} disabled={disabled}
    onChange={(event) => { setDraft(event.target.value); setInvalid(false); }} onBlur={commit}
    onKeyDown={(event) => {
      if (event.key === "Enter") { event.preventDefault(); commit(); }
      if (event.key === "Escape") { setDraft(String(value)); setInvalid(false); }
    }} />;
}
