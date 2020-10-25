exception UnexpectedResponse(int);

let handleApiError =
  [@bs.open]
  (
    fun
    | UnexpectedResponse(code) => code
  );

let acceptOrRejectResponse = response =>
  if (Fetch.Response.ok(response) || Fetch.Response.status(response) == 422) {
    response |> Fetch.Response.json;
  } else {
    Js.Promise.reject(UnexpectedResponse(response |> Fetch.Response.status));
  };

let handleResponseError = error => {
  Js.log("here");
  Js.log(error);
  let message = "Our team has been notified of this error. Please reload the page and try again.";

  switch (error) {
  | Some(code) when code == 401 =>
    Storage.deleteToken();
    Notification.notice(
      code |> string_of_int,
      "Login Expired please login again",
    );
  | Some(code) => Notification.error(code |> string_of_int, message)
  | None => Notification.error("An unexpected error occurred", message)
  };
};

let handleResponseJSON = (json, responseCB, errorCB) => {
  let error = json |> Json.Decode.(optional(field("error", string)));

  switch (error) {
  | Some(error) =>
    Notification.error("Something went wrong!", error);
    errorCB();
  | None => responseCB(json)
  };
};

let handleResponse = (responseCB, errorCB, promise) =>
  Js.Promise.(
    promise
    |> then_(response => acceptOrRejectResponse(response))
    |> then_(json =>
         handleResponseJSON(json, responseCB, errorCB) |> resolve
       )
    |> catch(error => {
         errorCB();
         Js.log(error);
         handleResponseError(error |> handleApiError) |> resolve;
       })
    |> ignore
  );

let sendPayload = (url, payload, responseCB, errorCB, method) =>
  Fetch.fetchWithInit(
    url,
    Fetch.RequestInit.make(
      ~method_=method,
      ~body=
        Fetch.BodyInit.make(Js.Json.stringify(Js.Json.object_(payload))),
      ~headers=Fetch.HeadersInit.make({"Content-Type": "application/json"}),
      ~credentials=Fetch.SameOrigin,
      (),
    ),
  )
  |> handleResponse(responseCB, errorCB);

let sendFormData = (url, formData, responseCB, errorCB) =>
  Fetch.fetchWithInit(
    url,
    Fetch.RequestInit.make(
      ~method_=Post,
      ~body=Fetch.BodyInit.makeWithFormData(formData),
      ~credentials=Fetch.SameOrigin,
      (),
    ),
  )
  |> handleResponse(responseCB, errorCB);

let create = (url, payload, responseCB, errorCB) =>
  sendPayload(url, payload, responseCB, errorCB, Post);

let update = (url, payload, responseCB, errorCB) =>
  sendPayload(url, payload, responseCB, errorCB, Patch);

let get = (url, responseCB, errorCB) =>
  Fetch.fetch(url) |> handleResponse(responseCB, errorCB);

let getWithToken = (url, token, responseCB, errorCB) =>
  Fetch.fetchWithInit(
    url,
    Fetch.RequestInit.make(
      ~method_=Get,
      ~headers=Fetch.HeadersInit.make({"authorization": "Bearer " ++ token}),
      ~credentials=Fetch.SameOrigin,
      (),
    ),
  )
  |> handleResponse(responseCB, errorCB);
