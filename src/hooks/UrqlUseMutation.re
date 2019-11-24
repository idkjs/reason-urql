/**
 * The type of executeMutation – a function returned by
 * useMutation to imperatively execute the mutation.
 */
type executeMutationJs =
  (
    option(Js.Json.t),
    option(UrqlClient.ClientTypes.partialOperationContext)
  ) =>
  Js.Promise.t(UrqlClient.ClientTypes.operationResult);

[@bs.module "urql"]
external useMutationJs:
  string => (UrqlTypes.jsResponse(Js.Json.t, 'extensions), executeMutationJs) =
  "useMutation";

/**
 * A function for converting the response to useQuery from the JavaScript
 * representation to a typed Reason record.
 */
let urqlResponseToReason =
    (
      parse: Js.Json.t => 'response,
      result: UrqlTypes.jsResponse(Js.Json.t, 'extensions),
    )
    : UrqlTypes.hookResponse('response, 'extensions) => {
  let data =
    result->UrqlTypes.jsDataGet->Js.Nullable.toOption->Belt.Option.map(parse);
  let error =
    result
    ->UrqlTypes.jsErrorGet
    ->Belt.Option.map(UrqlCombinedError.combinedErrorToRecord);
  let fetching = result->UrqlTypes.fetchingGet;
  let extensions = result->UrqlTypes.extensionsGet->Js.Nullable.toOption;

  let response =
    switch (fetching, data, error) {
    | (true, _, _) => UrqlTypes.Fetching
    | (false, _, Some(error)) => Error(error)
    | (false, Some(data), _) => Data(data)
    | (false, None, None) => NotFound
    };

  {fetching, data, error, response, extensions};
};

/**
 * The useMutation hook.
 *
 * Accepts the following arguments:
 *
 * request – a Js.t containing the query and variables corresponding
 * to the GraphQL mutation, and a parse function for decoding the JSON response.
 */;
let useMutation = (~request) => {
  let (responseJs, executeMutationJs) = useMutationJs(request##query);

  let response =
    React.useMemo2(
      () => responseJs |> urqlResponseToReason(request##parse),
      (request##parse, responseJs),
    );

  let executeMutation =
    React.useCallback1(
      context => executeMutationJs(Some(request##variables), context),
      [|request##variables|],
    );

  (response, executeMutation);
};

let useDynamicMutation = (~query, ~parse) => {
  let (responseJs, executeMutationJs) = useMutationJs(query);

  let response =
    React.useMemo1(
      () => responseJs |> urqlResponseToReason(parse),
      [|responseJs|],
    );

  let executeMutation =
    React.useCallback1(
      (request, context) =>
        executeMutationJs(Some(request##variables), context),
      [|executeMutationJs|],
    );

  (response, executeMutation);
};
